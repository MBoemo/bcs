//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
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

	public:
		inline void push( std::vector<int> i ){

			std::vector< std::vector< int > >::iterator pos = std::find(_arity2entries[i.size()].begin(),_arity2entries[i.size()].end(), i);
			if (pos != _arity2entries[i.size()].end()) return;

			if ( _arity2entries.count(i.size()) == 0 ){

				_arity2entries[i.size()] = {i};
			}			
			else{

				_arity2entries[i.size()].push_back(i);
			}
		}
		inline void pop( std::vector<int> i ){

			if ( _arity2entries.count( i.size() ) > 0 ){

				std::vector< std::vector< int > >::iterator pos = std::find(_arity2entries[i.size()].begin(),_arity2entries[i.size()].end(), i);

				if (pos != _arity2entries[i.size()].end()){

					_arity2entries[i.size()].erase(pos);
				}
			}
		}
		inline bool check( std::vector< std::vector< Token * > > setExpressions, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

			if (_arity2entries.count( setExpressions.size() ) == 0) return false;

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
		inline bool check_quick( std::vector< std::vector< Token * > > setExpressions, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

			if (_arity2entries.count( setExpressions.size() ) == 0) return false;

			std::vector< int > valueToFind;

			//get the one value that the beacon can check
			for ( unsigned int i = 0; i < setExpressions.size(); i++ ){

				Numerical n = evalRPN_numerical( setExpressions[i], param2value, _globalVars, localVariables );
				if (not n.isInt()) throw SyntaxError(setExpressions[i][0], "Set expressions must evaluate to ints, not floats.");
				valueToFind.push_back(n.getInt());
			}
			
			std::vector< std::vector< int > >::iterator pos = std::find(_arity2entries[setExpressions.size()].begin(), _arity2entries[setExpressions.size()].end(), valueToFind );

			if ( pos != _arity2entries[setExpressions.size()].end() ) return true;
			else return false;
		}
		inline std::vector< std::vector< int > > findAll( std::vector< std::vector< Token * > > setExpressions, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

			std::vector< std::vector< int > > out;

			if (_arity2entries.count( setExpressions.size() ) == 0) return out;

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
		inline std::vector< std::vector< int > > findAll_trivial( std::vector< std::vector< Token * > > setExpressions, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

			std::vector< std::vector< int > > out;

			if (_arity2entries.count( setExpressions.size() ) == 0) return out;

			std::vector< int > value;

			//get the one value that the beacon can check
			for ( unsigned int i = 0; i < setExpressions.size(); i++ ){

				Numerical n = evalRPN_numerical( setExpressions[i], param2value, _globalVars, localVariables );
				if (not n.isInt()) throw SyntaxError(setExpressions[i][0], "Set expressions must evaluate to ints, not floats.");
				value.push_back(n.getInt());
			}

			std::vector< std::vector< int > >::iterator pos = std::find(_arity2entries[setExpressions.size()].begin(), _arity2entries[setExpressions.size()].end(), value );

			if ( pos != _arity2entries[setExpressions.size()].end() ){
				out.push_back(value);
				return out;
			}
			else return out;
		}

		void printContents( void ){ //for testing

			std::cout << ">>>>>>>>>>>>DATABASE CONTENTS: ";

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
};


#endif
