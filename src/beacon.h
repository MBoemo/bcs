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


struct BetweenBounds {

	BetweenBounds( std::vector< std::vector< std::pair<int, int> > > i ) : i_ {i} {}
	bool operator()(std::vector< int > i) {

		for ( unsigned int dim = 0; dim < i.size(); dim++ ){ //go through dimensions

			bool dimMatch = false;

			for ( auto b = i_[dim].begin(); b < i_[dim].end(); b++ ){ //go through the pairs of bounds that we have

				//testing
				//std::cout << "<<<< " << (*b).first << " " << i[dim] << " " << (*b).second << std::endl;

				if ( (*b).first <= i[dim] and i[dim] <= (*b).second ){

					dimMatch = true;
					break;
				}
			}

			if ( not dimMatch ) return false;
		}

		return true;
	}
	std::vector< std::vector< std::pair<int, int> > > i_;
};


class communicationDatabase{

	private:
		std::map< int, std::vector< std::vector< int > > > _arity2entries;
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
		bool check( std::vector< std::vector< Token * > > setExpressions, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

			//std::cout << "in check" << std::endl;
			//if (_arity2entries.count( setExpressions.size() ) == 0) return false;

			std::vector< std::vector< std::pair<int, int> > > bounds;

			//get the bounds for each set expression
			for ( unsigned int i = 0; i < setExpressions.size(); i++ ){

				std::vector< std::pair<int, int > > b = evalRPN_set( setExpressions[i], param2value, _globalVars, localVariables );
				bounds.push_back(b);
			}

			std::vector< std::vector< int > >::iterator pos = std::find_if(_arity2entries[setExpressions.size()].begin(), _arity2entries[setExpressions.size()].end(), BetweenBounds(bounds) );

			if ( pos != _arity2entries[setExpressions.size()].end() ) return true;
			else return false;
		}
		bool check_quick(std::vector< int > &query){

			return Tree.search(query,Tree.getRoot());
		}
		std::vector< std::vector< int > > findAll( std::vector< std::vector< Token * > > setExpressions, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

			//std::cout << "in findAll" << std::endl;
			std::vector< std::vector< int > > out;

			//if (_arity2entries.count( setExpressions.size() ) == 0) return out;

			std::vector< std::vector< std::pair<int, int> > > bounds;

			//get the bounds for each set expression
			for ( unsigned int i = 0; i < setExpressions.size(); i++ ){

				std::vector< std::pair<int, int > > b = evalRPN_set( setExpressions[i], param2value, _globalVars, localVariables );
				bounds.push_back(b);
			}

			std::vector< std::vector< int > >::iterator pos = _arity2entries[setExpressions.size()].begin();
			while (pos != _arity2entries[setExpressions.size()].end()){

				pos = std::find_if(pos, _arity2entries[setExpressions.size()].end(), BetweenBounds(bounds) );

				if ( pos != _arity2entries[setExpressions.size()].end() ){

					out.push_back(*pos);
					pos++;
				}
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

		void printContents( void ){ //for testing

			std::cout << "   >>DATABASE CONTENTS: ";

			for ( auto dbValues = _arity2entries.begin(); dbValues != _arity2entries.end(); dbValues++ ){ //go through arities	

				for ( auto entry = (dbValues -> second).begin(); entry < (dbValues -> second).end(); entry++ ){

					for ( unsigned int i = 0; i < (*entry).size(); i++ ) std::cout << (*entry)[i] << " ";

				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
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
