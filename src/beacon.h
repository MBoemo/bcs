//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef BEACON_H
#define BEACON_H

#include <map>
#include <memory>
#include <map>
#include <chrono>
#include <list>
#include <iomanip>
#include <sstream>
#include <iterator>
#include "evaluate_trees.h"
#include "BPTree.h"


class communicationDatabase{

	private:
		GlobalVariables _globalVars;
		BPTree<int> _UnaryTree;
		std::map<unsigned int, BPTree<std::vector<int>>> _arity2Tree;

	public:
		void push( std::vector<int> i ){
			//std::cout << "in push" << std::endl;

			if (i.size() == 1){
				databaseEntry<int> *e = new databaseEntry<int>;
				e -> entry = i[0];
				_UnaryTree.insertEntry(e);
			}
			else{

				databaseEntry<std::vector<int>> *e = new databaseEntry<std::vector<int>>;
				e -> entry = i;
				_arity2Tree[i.size()].insertEntry(e);
			}
		}
		void pop( std::vector<int> i ){
			//std::cout << "in pop" << std::endl;

			if (i.size() == 1) _UnaryTree.deleteEntry(i[0]);
			else _arity2Tree[i.size()].deleteEntry(i);
		}
		bool check( std::vector< std::vector< int> > &lb, std::vector< std::vector< int> > &ub){

			assert(lb.size() == ub.size());

			if (lb[0].size() == 1){
				for (size_t i = 0; i < lb.size(); i++){
					if (_UnaryTree.search_bounds(lb[i][0], ub[i][0], _UnaryTree.getRoot())) return true;
				}
				return false;

			}
			else{
				for (size_t i = 0; i < lb.size(); i++){
					if (_arity2Tree[lb[0].size()].search_bounds(lb[i], ub[i], _arity2Tree[lb[0].size()].getRoot())) return true;
				}
				return false;
			}
		}
		bool check_quick(std::vector< int > &query){

			if (query.size() == 1){

				return _UnaryTree.search(query[0],_UnaryTree.getRoot());
			}
			else{

				return _arity2Tree[query.size()].search(query,_arity2Tree[query.size()].getRoot());
			}
		}
		std::vector< std::vector< int > > findAll( std::vector< std::vector< int> > &lb, std::vector< std::vector< int> > &ub ){

			assert(lb.size() == ub.size());
			std::vector< std::vector< int > > out;

			if (lb[0].size() == 1){

				std::vector< int > temp;
				for (size_t i = 0; i < lb.size(); i++){
					_UnaryTree.search_boundsReturnAll(lb[i][0], ub[i][0], _UnaryTree.getRoot(), temp);
				}
				for (auto t = temp.begin(); t < temp.end(); t++) out.push_back({*t}); //TODO: this is a bodge in lieu of making this a template
				return out;
			}
			else{

				for (size_t i = 0; i < lb.size(); i++){
					_arity2Tree[lb[0].size()].search_boundsReturnAll(lb[i], ub[i], _arity2Tree[lb[0].size()].getRoot(), out);
				}
				return out;
			}
		}
		std::vector< std::vector< int > > findAll_trivial( std::vector< int > &query ){

			//std::cout << "in findAll_trivial" << std::endl;
			std::vector< std::vector< int > > out;

			if (query.size() == 1){
				if (_UnaryTree.search(query[0], _UnaryTree.getRoot())){
					out.push_back({query[0]});
					return out;
				}
				else{
					return out;
				}
			}
			else{

				if (_arity2Tree[query.size()].search(query, _arity2Tree[query.size()].getRoot())){
					out.push_back(query);
					return out;
				}
				else{
					return out;
				}
			}
		}
};


class BeaconChannel{

	private:
		std::vector< std::string > _channelName;
		communicationDatabase _database;
		GlobalVariables _globalVars;
		std::map< SystemProcess *, std::list< std::shared_ptr<Candidate> > > _potentialBeaconReceiveCands;
		std::map< SystemProcess *, std::list< std::shared_ptr<Candidate> > > _activeBeaconReceiveCands;
		std::map< SystemProcess *, std::list< std::shared_ptr<Candidate> > > _sendCands;

	public:
		BeaconChannel( std::vector< std::string >, GlobalVariables & );
		BeaconChannel( const BeaconChannel & );
		std::vector< std::string > getChannelName(void);
		void updateBeaconCandidates(int &, double &);
		void cleanSPFromChannel( SystemProcess *, int &, double & );
		std::shared_ptr<Candidate> pickCandidate(double &, double, double);
		void addCandidate( Block *, SystemProcess *, std::list< SystemProcess > , ParameterValues &, int &, double & );
		bool matchClone( SystemProcess *, SystemProcess *);
		void cleanCloneFromChannel( SystemProcess * );
};


#endif
