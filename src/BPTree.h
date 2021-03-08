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

//T is either going to be an int or std::vector<int> depending on whether


template <class T>
class databaseEntry {
	public:
		T entry;
};

template <class T>
class BPNode {

	public:
		std::vector<T> key;
		std::vector<BPNode<T> *> Ptree;
		std::vector<databaseEntry<T> *> Pdata;
		BPNode<T> *parent;
		bool isLeaf, isRoot;
};

template <class T>
class BPTree {

	private:
		//this should stay even so that splitting nodes works out nicely
		unsigned int BP_MAX=100;
		BPNode<T> *_root;
		bool _isEmpty = true;
		BPNode<T> *searchReturn(T &query, BPNode<T> *cursor){

			//trivial exit if the root is also a leaf
			if (cursor -> isLeaf) return cursor;

			auto it = std::upper_bound((cursor -> key).begin(),(cursor -> key).end(), query);
			if (it != (cursor -> key).end()){

				unsigned int index = it - (cursor -> key).begin();

				if ((cursor -> Ptree)[index] -> isLeaf){//if we found a leaf node, then we're done

					return (cursor -> Ptree)[index];
				}
				else{//otherwise keep recursing down

					return searchReturn(query, (cursor -> Ptree)[index]);
				}
			}
			else{
				assert(query >= (cursor -> key).back());
				if ((cursor -> Ptree).back() -> isLeaf){//if we found a leaf node, then we're done

					return (cursor -> Ptree).back();
				}
				else{//otherwise keep recursing down

					return searchReturn(query, (cursor -> Ptree).back());
				}
			}
		}
		void rebalance(BPNode<T> *cursor){
			//std::cout << "in rebalance" << std::endl;


			BPNode<T> *newNode = new BPNode<T>;
			newNode -> isRoot = false;

			//we're going to push this value up one level
			T pushUp = (cursor -> key)[BP_MAX/2 + 1];

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
				std::vector< T > key_lo((cursor -> key).begin(), (cursor -> key).begin() + BP_MAX/2);
				std::vector< T > key_hi((cursor -> key).begin() + BP_MAX/2, (cursor -> key).end());
				std::vector<databaseEntry<T> *> Pdata_lo((cursor -> Pdata).begin(), (cursor -> Pdata).begin() + BP_MAX/2);
				std::vector<databaseEntry<T> *> Pdata_hi((cursor -> Pdata).begin() + BP_MAX/2, (cursor -> Pdata).end());

				cursor -> key = key_lo;
				cursor -> Pdata = Pdata_lo;
				newNode -> key = key_hi;
				newNode -> Pdata = Pdata_hi;
			}
			else{
				//for internal nodes, drop the value at BP_MAX/2 because we're pushing it up to the parent

				newNode -> isLeaf = false;

				//split into halves, existing node keeps the lower half
				std::vector< T > key_lo((cursor -> key).begin(), (cursor -> key).begin() + BP_MAX/2);
				std::vector< T > key_hi((cursor -> key).begin() + BP_MAX/2 + 1, (cursor -> key).end());
				std::vector<BPNode<T> *> Ptree_lo((cursor -> Ptree).begin(), (cursor -> Ptree).begin() + BP_MAX/2);
				std::vector<BPNode<T> *> Ptree_hi((cursor -> Ptree).begin() + BP_MAX/2, (cursor -> Ptree).end());

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
				BPNode<T> *newRoot = new BPNode<T>;
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

				BPNode<T> *parent = cursor -> parent;

				if (pushUp > (parent -> key).back()){

					(parent -> key).push_back(pushUp);
					(parent -> Ptree).push_back(newNode);
				}
				else{

					auto it = std::upper_bound((parent -> key).begin(),(parent -> key).end(), pushUp);
					unsigned int index = it - (parent -> key).begin();
					(parent -> key).insert((parent -> key).begin()+index,pushUp);
					(parent -> Ptree).insert((parent -> Ptree).begin()+index+1,newNode);//the +1 here is because we want to insert to the right of the key
				}
				newNode -> parent = parent;

				//continue rebalancing the tree from the parent if we have to
				if ( (parent -> key).size() > BP_MAX ){

					rebalance(parent);
				}
			}
		}
		T getSubtreeMinimum(BPNode<T> *cursor){

			if (cursor -> isLeaf){

				//return the value in the minimum data pointer
				databaseEntry<T> *leftMost = (cursor -> Pdata)[0];
				return leftMost -> entry;
			}
			else{

				//keep recursing down through the leftmost child
				return getSubtreeMinimum((cursor -> Ptree)[0]);
			}
		}
		void manageUnderflow(T &query, BPNode<T> *cursor){

			auto it = std::find( (cursor -> key).begin(), (cursor -> key).end(), query );
			if (it != (cursor -> key).end()){ //the query is in the node

				//delete the query from the node
				int index = it - (cursor -> key).begin();
				assert(index >= 0);
				(cursor -> key).erase((cursor -> key).begin() + index);

				if (cursor -> isLeaf){ //for leaves, we're going to delete data pointers
					delete (cursor -> Pdata)[index];
					(cursor -> Pdata).erase((cursor -> Pdata).begin() + index);
				}
				else{ //for internal nodes, we're going to delete tree pointers
					(cursor -> Ptree).erase((cursor -> Ptree).begin() + index);
				}

				//check if the deletion created underflow
				if (cursor -> isRoot ){

					if ((cursor -> key).size() > 0){ //the root is special and only needs one key
						return;
					}
					else if (not cursor -> isLeaf){
						//we're about to delete the only key in the root, so we need a new root
						//if the root also happens to be a leaf, however, we're allowed to go to zero (empty tree)

						assert( ((cursor -> Ptree).size() == 2) and ((cursor -> key).size() == 1) );

						//get the minimum data value in the right subtree
						T newRootKey = getSubtreeMinimum((cursor -> Ptree)[1]);

						//make that value the new root
						(cursor -> key).clear();
						(cursor -> key).push_back(newRootKey);
					}
					else{
						//the root was also a leaf and its size is now zero, so we have an empty tree
						assert(not _isEmpty);
						delete cursor;
						_isEmpty = true;
					}
				}
				else if ((cursor -> key).size() < BP_MAX/2){ //non-root

					//get the neighbouring nodes
					bool hasLeft;
					bool hasRight;
					BPNode<T> *leftNeighbour, *rightNeighbour;
					auto it = std::find(((cursor -> parent) -> Ptree).begin(), ((cursor -> parent) -> Ptree).end(), cursor);
					unsigned int Ptree_index = it - ((cursor -> parent) -> Ptree).begin();
					if (Ptree_index > 0){ //has a left neighbour
						hasLeft = true;
						leftNeighbour = ((cursor -> parent) -> Ptree)[Ptree_index-1];
					}
					if (Ptree_index < ((cursor -> parent) -> Ptree).size() - 1){ //has a right neighbour
						hasRight = true;
						rightNeighbour = ((cursor -> parent) -> Ptree)[Ptree_index+1];
					}

					//this isn't the root, so we should have a neighbour
					assert(hasLeft or hasRight);

					//check to see if there's a neighbouring node we can borrow a key from
					bool borrowedKey = false;
					if (hasLeft){

						if ( (leftNeighbour -> key).size() > BP_MAX/2){ //left neighbour can donate a key

							(cursor -> key).insert((cursor -> key).begin(), (leftNeighbour -> key).back());
							(leftNeighbour -> key).pop_back();

							if (cursor -> isLeaf){ //borrow a data pointer

								(cursor -> Pdata).insert((cursor -> Pdata).begin(), (leftNeighbour -> Pdata).back());
								(leftNeighbour -> Pdata).pop_back();
							}
							else{ //borrow a tree node pointer

								(cursor -> Ptree).insert((cursor -> Ptree).begin(), (leftNeighbour -> Ptree).back());
								(leftNeighbour -> Ptree).pop_back();
							}

							borrowedKey = true;
						}

					}
					else{ //hasRight

						if ( (rightNeighbour -> key).size() > BP_MAX/2){ //right neighbour can donate a key

							(cursor -> key).push_back((rightNeighbour -> key).front());
							(rightNeighbour -> key).erase((rightNeighbour -> key).begin());

							if (cursor -> isLeaf){ //borrow a data pointer

								(cursor -> Pdata).push_back((rightNeighbour -> Pdata).front());
								(rightNeighbour -> Pdata).erase((rightNeighbour -> Pdata).begin());
							}
							else{ //borrow a tree node pointer

								(cursor -> Ptree).push_back((rightNeighbour -> Ptree).front());
								(rightNeighbour -> Ptree).erase((rightNeighbour -> Ptree).begin());
							}

							borrowedKey = true;
						}
					}

					//if we can't borrow, we need to merge the node with a neighbour
					if (not borrowedKey){

						if (hasLeft){

							//get the key from the parent that separates the cursor node from the left neighbour
							T separatingKey = (cursor -> parent -> key)[Ptree_index-1];
							assert(separatingKey != (cursor -> key).front() and separatingKey != (leftNeighbour -> key).back());

							//merge the keys together
							(cursor -> key).insert((cursor -> key).begin(),separatingKey);
							(cursor -> key).insert((cursor -> key).begin(), (leftNeighbour -> key).begin(), (leftNeighbour -> key).end());

							if (cursor -> isLeaf){ //merge data pointers

								(cursor -> Pdata).insert((cursor -> Pdata).begin(), (leftNeighbour -> Pdata).begin(), (leftNeighbour -> Pdata).end());
							}
							else{ //merge tree pointers

								(cursor -> Ptree).insert((cursor -> Ptree).begin(), (leftNeighbour -> Ptree).begin(), (leftNeighbour -> Ptree).end());

								//tell the node pointers who the new parent is
								for (auto n = (cursor -> Ptree).begin(); n < (cursor -> Ptree).end(); n++){

									(*n) -> parent = cursor;
								}
							}

							delete leftNeighbour;

							//clean up the parent - delete the key that we pulled down and the pointer to the now-deleted left neighbour
							(cursor -> parent -> key).erase( (cursor -> parent -> key).begin() + Ptree_index-1);
							(cursor -> parent -> Ptree).erase( (cursor -> parent -> Ptree).begin() + Ptree_index-1);
						}
						else{ //hasRight

							//get the key from the parent that separates the cursor node from the left neighbour
							T separatingKey = (cursor -> parent -> key)[Ptree_index];
							assert(separatingKey != (cursor -> key).back() and separatingKey != (rightNeighbour -> key).front());

							//merge the keys together
							(cursor -> key).push_back(separatingKey);
							(cursor -> key).insert((cursor -> key).end(), (rightNeighbour -> key).begin(), (rightNeighbour -> key).end());

							if (cursor -> isLeaf){ //merge data pointers

								(cursor -> Pdata).insert((cursor -> Pdata).end(), (rightNeighbour -> Pdata).begin(), (rightNeighbour -> Pdata).end());
							}
							else{ //merge tree pointers

								(cursor -> Ptree).insert((cursor -> Ptree).end(), (rightNeighbour -> Ptree).begin(), (rightNeighbour -> Ptree).end());

								//tell the node pointers who the new parent is
								for (auto n = (cursor -> Ptree).begin(); n < (cursor -> Ptree).end(); n++){

									(*n) -> parent = cursor;
								}
							}

							delete rightNeighbour;

							//clean up the parent - delete the key that we pulled down and the pointer to the now-deleted left neighbour
							(cursor -> parent -> key).erase( (cursor -> parent -> key).begin() + Ptree_index);
							(cursor -> parent -> Ptree).erase( (cursor -> parent -> Ptree).begin() + Ptree_index+1);
						}

						//if the parent is the root and we took its only key, delete it and re-root the tree
						if (cursor -> parent -> isRoot and (cursor -> parent -> key).size() == 0){

							delete cursor -> parent;
							cursor -> isRoot = true;
						}
					}

					assert( ((cursor -> key).size() >= BP_MAX/2) and ((cursor -> key).size() <= BP_MAX) );
					if (not cursor -> isLeaf) assert( (cursor -> key).size() + 1 == (cursor -> Ptree).size() );
					if (cursor -> isLeaf) assert( (cursor -> key).size() + 1 == (cursor -> Pdata).size() );

					manageUnderflow(query, cursor -> parent);
				}
				else if (not cursor -> isRoot){ //keep recursing up if there's no underflow
					manageUnderflow(query, cursor -> parent);
				}
			}
			else if (not cursor -> isRoot){ //nothing to delete so keep recursing up
				manageUnderflow(query, cursor -> parent);
			}
		}
		void recursiveDestroy(BPNode<T> *cursor){

			if (not cursor -> isLeaf){

				for (auto n = (cursor -> Ptree).begin(); n < (cursor -> Ptree).end(); n++){

						recursiveDestroy(*n);
				}
				delete cursor;
			}
			else{

				for (auto n = (cursor -> Pdata).begin(); n < (cursor -> Pdata).end(); n++){

						delete *n;
				}
				delete cursor;
			}
		}

	public:
		~BPTree(){
			if (not _isEmpty){
				recursiveDestroy(_root);
			}
		}
		void insertEntry(databaseEntry<T> *de){
			//std::cout << "in insertEntry" << std::endl;

			if (_isEmpty){//base case where we don't have any nodes yet

				//make a root node
				BPNode<T> *node = new BPNode<T>;
				_root = node;
				node -> isLeaf = true;
				node -> isRoot = true;
				(node -> key).push_back(de -> entry);
				(node -> Pdata).push_back(de);
				_isEmpty = false;
			}
			else{

				//get the leaf node this entry should be in
				BPNode<T> *targetLeaf = searchReturn(de -> entry, _root);

				//abort if the entry is already in the leaf
				if (std::binary_search( (targetLeaf -> key).begin(), (targetLeaf -> key).end(), de -> entry )) return;

				//insert into the leaf
				if (de -> entry > (targetLeaf -> key).back()){ //insert at the end

					(targetLeaf -> key).push_back(de -> entry);
					(targetLeaf -> Pdata).push_back(de);
				}
				else{ //insert internally

					auto it = std::upper_bound((targetLeaf -> key).begin(),(targetLeaf -> key).end(), de -> entry);
					unsigned int index = it - (targetLeaf -> key).begin();
					(targetLeaf -> key).insert((targetLeaf -> key).begin()+index, de -> entry);
					(targetLeaf -> Pdata).insert((targetLeaf -> Pdata).begin()+index,de);
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
		void deleteEntry(T &query){

			//get the leaf where the entry to delete would live
			//if the entry to delete isn't actually in the database, don't do anything
			if (_isEmpty) return;
			BPNode<T> *targetLeaf = searchReturn(query, _root);
			if (not search(query, targetLeaf)) return;

			//delete the entry and correct any underflow by recursing up the tree to the root
			manageUnderflow(query, targetLeaf);
		}
		bool search(T &query, BPNode<T> *cursor){
			//std::cout << "in search" << std::endl;

			//trivial exit if the tree is empty
			if (_isEmpty) return false;

			if (cursor -> isLeaf){ //if we've reached a leaf, then the query should be one of the keys if it's in the database at all

				if (std::binary_search( (cursor -> key).begin(), (cursor -> key).end(), query )){
					return true;
				}
				else{
					return false;
				}
			}
			else{ //if we're not in a leaf, keep recursing down

				auto it = std::upper_bound((cursor -> key).begin(),(cursor -> key).end(), query);
				if (it != (cursor -> key).end()){

					unsigned int index = it - (cursor -> key).begin();

					//stop early if we find the query in an internal node
					if (query == (cursor -> key)[index]) return true;

					return search(query, (cursor -> Ptree)[index]);
				}
				else{
					//if we haven't recursed by now, then the query is bigger than the last key
					assert(query >= (cursor -> key).back());
					return search(query, (cursor -> Ptree).back());
				}
			}
		}
		bool search_bounds(T &lb, T &ub, BPNode<T> *cursor){
			//std::cout << "in search bounds" << std::endl;

			//trivial exit if the tree is empty
			if (_isEmpty) return false;

			if (cursor -> isLeaf){
				//if we've reached a leaf, then return true if there's any overlap at all between the key range and [lb,ub]

				if (ub < (cursor -> key).front()) return false;
				else if ((cursor -> key).back() < lb) return false;
				else{
					//TODO: replace with binary search
					for (size_t i = 0; i < (cursor -> key).size(); i++){

						if (lb <= (cursor -> key)[i] and (cursor -> key)[i] <= ub){
							return true;
						}
					}
					return false;
				}
			}
			else{ //if we're not in a leaf, keep recursing down

				if (ub < (cursor -> key).front()){

					return search_bounds(lb, ub, (cursor -> Ptree).front());
				}
				else if ((cursor -> key).back() <= lb){

					return search_bounds(lb, ub, (cursor -> Ptree).back());
				}
				else{

					//find the tree pointer that contains the appropriate node for lb
					auto lb_it = std::upper_bound((cursor -> key).begin(),(cursor -> key).end(), lb);
					assert( lb_it != (cursor -> key).end());
					ssize_t lbIndex = lb_it - (cursor -> key).begin();

					//find the tree pointer that contains the appropriate node for ub
					ssize_t ubIndex = -1;
					if (ub >= (cursor -> key).back()){
						ubIndex = (cursor -> Ptree).size()-1;
					}
					else{

						auto ub_it = std::upper_bound((cursor -> key).begin(),(cursor -> key).end(), ub);
						assert( ub_it != (cursor -> key).end());
						ubIndex = ub_it - (cursor -> key).begin();
					}
					assert(ubIndex != -1);

					if (ubIndex != lbIndex){
						//if a node key lies in [lb,ub], then so will a data point and we're done
						return true;
					}
					else if ((ub == lb) and (lb == (cursor -> key)[lbIndex])){
						//if we're only looking for one value and we've found it in an internal node, we're done
						return true;
					}
					else{
						//the only case where we keep recursing down is if lbIndex == ubIndex, because otherwise we would have a key in [lb,ub]
						assert(lbIndex == ubIndex);
						return search_bounds(lb, ub, (cursor -> Ptree)[lbIndex]);
					}
				}
			}
		}
		void search_boundsReturnAll(T &lb, T &ub, BPNode<T> *cursor, std::vector<T> &values){
			//std::cout << "in search bounds" << std::endl;

			//trivial exit if the tree is empty
			if (_isEmpty) return;

			if (cursor -> isLeaf){
				//if we've reached a leaf, then return true if there's any overlap at all between the key range and [lb,ub]

				if (ub < (cursor -> key).front()) return;
				else if ((cursor -> key).back() < lb) return;
				else{
					//TODO: replace with binary search
					for (size_t i = 0; i < (cursor -> key).size(); i++){

						if (lb <= (cursor -> key)[i] and (cursor -> key)[i] <= ub){

							values.push_back((cursor -> Pdata)[i] -> entry);
						}
					}
					return;
				}
			}
			else{ //if we're not in a leaf, keep recursing down

				if (ub < (cursor -> key).front()){

					search_boundsReturnAll(lb, ub, (cursor -> Ptree).front(), values);
				}
				else if ((cursor -> key).back() <= lb){

					search_boundsReturnAll(lb, ub, (cursor -> Ptree).back(), values);
				}
				else{

					//find the tree pointer that contains the appropriate node for lb
					auto lb_it = std::upper_bound((cursor -> key).begin(),(cursor -> key).end(), lb);
					assert( lb_it != (cursor -> key).end());
					ssize_t lbIndex = lb_it - (cursor -> key).begin();

					//find the tree pointer that contains the appropriate node for ub
					ssize_t ubIndex = -1;
					if (ub >= (cursor -> key).back()){
						ubIndex = (cursor -> Ptree).size()-1;
					}
					else{

						auto ub_it = std::upper_bound((cursor -> key).begin(),(cursor -> key).end(), ub);
						assert( ub_it != (cursor -> key).end());
						ubIndex = ub_it - (cursor -> key).begin();
					}
					assert(ubIndex != -1);

					for (ssize_t i = lbIndex; i < ubIndex; i++){

						search_boundsReturnAll(lb, ub, (cursor -> Ptree)[i], values);
					}
				}
			}
		}
		BPNode<T> *getRoot(void){
			return _root;
		}
};

#endif /* SRC_BPTREE_H_ */
