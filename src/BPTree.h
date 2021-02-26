//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG_BPTREE 1

#ifndef SRC_BPTREE_H_
#define SRC_BPTREE_H_

#include <iostream>
#include <cassert>
#include <utility>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "lexer.h"
#include "error_handling.h"
#include "parser.h"

class databaseEntry {
	public:
		std::vector<int> entry;
};

class BPNode {

	public:
		std::vector<int> key;
		std::vector<BPNode *> Ptree;
		std::vector<databaseEntry *> Pdata;
		BPNode *parent;
		bool isLeaf, isRoot;
};

class BPTree {

	private:
		//this should stay even so that splitting nodes works out nicely
		unsigned int BP_MAX=100;
		BPNode *_root;
		std::vector<BPNode *> _allNodes;
		std::vector<databaseEntry *> _allData;
		BPNode* searchReturn(std::vector<int> & query, BPNode *cursor){
			//std::cout << "in searchReturn" << std::endl;

			//trivial exit if the root is also a leaf
			if (cursor -> isLeaf) return cursor;

			for (size_t i = 0; i < (cursor -> key).size(); i++){

				if (query[0] < (cursor -> key)[i]){

					if ((cursor -> Ptree)[i] -> isLeaf){//if we found a leaf node, then we're done

						return (cursor -> Ptree)[i];
					}
					else{//otherwise keep recursing down

						return searchReturn(query, (cursor -> Ptree)[i]);
					}
				}
			}

			//if we haven't recursed by now, then the query is bigger than the last key
			assert(query[0] >= (cursor -> key).back());
			if ((cursor -> Ptree).back() -> isLeaf){//if we found a leaf node, then we're done

				return (cursor -> Ptree).back();
			}
			else{//otherwise keep recursing down

				return searchReturn(query, (cursor -> Ptree).back());
			}
		}
		void rebalance(BPNode *cursor){
			//std::cout << "in rebalance" << std::endl;


			BPNode *newNode = new BPNode;
			newNode -> isRoot = false;
			_allNodes.push_back(newNode);

			//we're going to push this value up one level
			int pushUp = (cursor -> key)[BP_MAX/2 + 1];

#if DEBUG_BPTREE
std::cout << "Rebalancing, pushing up: " << pushUp << std::endl;
#endif

			//split the node
			if (cursor -> isLeaf){
				//leaves need special handing:
				// - unlike in the internal nodes, we want to keep the value at the BP_MAX/2, otherwise we lose data
				// - no Ptree pointers so don't bother, split the Pdata pointers instead

				newNode -> isLeaf = true;

				//split into halves, existing node keeps the lower half
				std::vector<int> key_lo((cursor -> key).begin(), (cursor -> key).begin() + BP_MAX/2);
				std::vector<int> key_hi((cursor -> key).begin() + BP_MAX/2, (cursor -> key).end());
				std::vector<databaseEntry *> Pdata_lo((cursor -> Pdata).begin(), (cursor -> Pdata).begin() + BP_MAX/2);
				std::vector<databaseEntry *> Pdata_hi((cursor -> Pdata).begin() + BP_MAX/2, (cursor -> Pdata).end());

				cursor -> key = key_lo;
				cursor -> Pdata = Pdata_lo;
				newNode -> key = key_hi;
				newNode -> Pdata = Pdata_hi;
			}
			else{
				//for internal nodes, drop the value at BP_MAX/2 because we're pushing it up to the parent

				newNode -> isLeaf = false;

				//split into halves, existing node keeps the lower half
				std::vector<int> key_lo((cursor -> key).begin(), (cursor -> key).begin() + BP_MAX/2);
				std::vector<int> key_hi((cursor -> key).begin() + BP_MAX/2 + 1, (cursor -> key).end());
				std::vector<BPNode *> Ptree_lo((cursor -> Ptree).begin(), (cursor -> Ptree).begin() + BP_MAX/2);
				std::vector<BPNode *> Ptree_hi((cursor -> Ptree).begin() + BP_MAX/2, (cursor -> Ptree).end());

				cursor -> key = key_lo;
				cursor -> Ptree = Ptree_lo;
				newNode -> key = key_hi;
				newNode -> Ptree = Ptree_hi;

				//children of the new node now have a new parent
				for (auto i = Ptree_hi.begin(); i < Ptree_hi.end(); i++){
					(*i) -> parent = newNode;
				}
			}

			//push a value up to the parent
			if (cursor -> isRoot){

				//the root won't have a parent, so we need to make one and re-root the tree
				BPNode *newRoot = new BPNode;
				_allNodes.push_back(newRoot);
				cursor -> isRoot = false;
				newRoot -> isRoot = true;
				newRoot -> isLeaf = false;
				_root = newRoot;

				newRoot -> key = {pushUp};
				newRoot -> Ptree = {cursor,newNode};

				//the split cursor nodes now have the new root as their parent
				cursor -> parent = newRoot;
				newNode -> parent = newRoot;
			}
			else{

				BPNode *parent = cursor -> parent;

				if (pushUp > (parent -> key).back()){

					(parent -> key).push_back(pushUp);
					(parent -> Ptree).push_back(newNode);
				}
				else{
					for (size_t i = 0; i < (parent -> key).size(); i++){

						if (pushUp < (parent -> key)[i]){

							//insert the key and data pointer
							(parent -> key).insert((parent -> key).begin()+i,pushUp);
							(parent -> Ptree).insert((parent -> Ptree).begin()+i+1,newNode);//the +1 here is because we want to insert to the right of the key
							break;
						}
					}
				}
				newNode -> parent = parent;

				//continue rebalancing the tree from the parent if we have to
				if ( (parent -> key).size() > BP_MAX ){

					rebalance(parent);
				}
			}
		}

	public:
		void insertEntry(databaseEntry *de){
			//std::cout << "in insertEntry" << std::endl;

			if (_allNodes.size() == 0){//base case where we don't have any nodes yet

				//make a root node
				BPNode *node = new BPNode;
				_root = node;
				node -> isLeaf = true;
				node -> isRoot = true;
				(node -> key).push_back((de -> entry)[0]);
				(node -> Pdata).push_back(de);
				_allNodes.push_back(node);
				_allData.push_back(de);
			}
			else{

				//get the leaf node this entry should be in
				BPNode *targetLeaf = searchReturn(de -> entry, _root);

				//abort if the entry is already in the leaf
				if (std::find( (targetLeaf -> key).begin(), (targetLeaf -> key).end(), (de -> entry)[0] ) != (targetLeaf -> key).end()) return;
				_allData.push_back(de);

				//insert into the leaf
				if ((de -> entry)[0] > (targetLeaf -> key).back()){ //insert at the end

					(targetLeaf -> key).push_back((de -> entry)[0]);
					(targetLeaf -> Pdata).push_back(de);
				}
				else{ //insert internally
					for (size_t i = 0; i < (targetLeaf -> key).size(); i++){

						if ((de -> entry)[0] < (targetLeaf -> key)[i]){

							//insert the key and data pointer
							(targetLeaf -> key).insert((targetLeaf -> key).begin()+i,(de -> entry)[0]);
							(targetLeaf -> Pdata).insert((targetLeaf -> Pdata).begin()+i,de);
							break;
						}
					}
				}

				//rebalance the tree from the leaf if we have to
				if ( (targetLeaf -> key).size() > BP_MAX ){

					rebalance(targetLeaf);
				}
			}
#if DEBUG_BPTREE
std::cout << "Added: " << (de -> entry)[0] << std::endl;
std::cout << "Total nodes: " << _allNodes.size() << std::endl;
std::cout << "Total data pointers: " << _allData.size() << std::endl;
#endif
		}
		void deleteEntry(databaseEntry &de){



		}
		bool search(std::vector<int> &query, BPNode *cursor){
			//std::cout << "in search" << std::endl;

			//trivial exit if the tree is empty
			if (_allNodes.size() == 0) return false;

			if (cursor -> isLeaf){ //if we've reached a leaf, then the query should be one of the keys if it's in the database at all

				if (std::find( (cursor -> key).begin(), (cursor -> key).end(), query[0] ) != (cursor -> key).end()){
					return true;
				}
				else{
					return false;
				}
			}
			else{ //if we're not in a leaf, keep recursing down

				for (size_t i = 0; i < (cursor -> key).size(); i++){

					if (query[0] <= (cursor -> key)[i]){

						return search(query, (cursor -> Ptree)[i]);
					}
				}

				//if we haven't recursed by now, then the query is bigger than the last key
				assert(query[0] > (cursor -> key).back());
				return search(query, (cursor -> Ptree).back());
			}
		}
		BPNode *getRoot(void){
			return _root;
		}
};



#endif /* SRC_BPTREE_H_ */
