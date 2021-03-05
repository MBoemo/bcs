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
		BPTree Tree;

	public:
		void push( std::vector<int> i ){
			//std::cout << "in push" << std::endl;

			databaseEntry *e = new databaseEntry;
			e -> entry = i;
			Tree.insertEntry(e);
		}
		void pop( std::vector<int> i ){
			//std::cout << "in pop" << std::endl;

			Tree.deleteEntry(i);
		}
		void getBoundsCombinations(std::vector< std::vector< std::pair<int, int> > > &input,
				unsigned int dim,
				std::vector<int> lbCarryOver,
				std::vector<int> ubCarryOver,
				std::vector<std::vector<int>> &lbOut,
				std::vector<std::vector<int>> &ubOut){

			for (size_t i = 0; i < input[dim].size(); i++){

				std::vector<int> lbAppend = lbCarryOver;
				std::vector<int> ubAppend = ubCarryOver;

				lbAppend.push_back(input[dim][i].first);
				ubAppend.push_back(input[dim][i].second);

				if (dim < input.size() - 1){

					getBoundsCombinations(input, dim+1, lbAppend, ubAppend, lbOut, ubOut);
				}
				else{

					assert(lbAppend.size() == ubAppend.size());
					lbOut.push_back(lbAppend);
					ubOut.push_back(ubAppend);
				}
			}
		}
		bool check( std::vector< std::vector< int> > &lb, std::vector< std::vector< int> > &ub){

			assert(lb.size() == ub.size());
			for (size_t i = 0; i < lb.size(); i++){
				if (Tree.search_bounds(lb[i], ub[i], Tree.getRoot())) return true;
			}
			return false;
		}
		bool check_quick(std::vector< int > &query){

			return Tree.search(query,Tree.getRoot());
		}
		std::vector< std::vector< int > > findAll( std::vector< std::vector< int> > &lb, std::vector< std::vector< int> > &ub ){

			assert(lb.size() == ub.size());

			std::vector< std::vector< int > > out;
			for (size_t i = 0; i < lb.size(); i++){
				Tree.search_boundsReturnAll(lb[i], ub[i], Tree.getRoot(), out);
			}
			return out;
		}
		std::vector< std::vector< int > > findAll_trivial( std::vector< int > &query ){

			//std::cout << "in findAll_trivial" << std::endl;
			std::vector< std::vector< int > > out;

			if (Tree.search(query,Tree.getRoot())){
				out.push_back(query);
				return out;
			}
			else{
				return out;
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
