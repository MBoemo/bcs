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


class communicationDatabase{

	private:
		std::map< std::vector< std::string >, std::vector< std::vector< int > > > _channel2values;
		GlobalVariables _globalVars;
	public:
		inline void push( std::vector< std::string > channel, std::vector<int> i ){

			if ( _channel2values.count( channel ) > 0 ){

				if ( std::find( (_channel2values[ channel ]).begin(), (_channel2values[ channel ]).end(), i ) == (_channel2values[ channel ]).end() ){

					(_channel2values[ channel ]).push_back( i );
				}
			}
			else{

				_channel2values[ channel ] = { i };
			}
		}
		inline void pop( std::vector< std::string > channel, std::vector<int> i ){

			if ( _channel2values.count( channel ) > 0 ){

				for ( auto posItr = (_channel2values[ channel ]).begin(); posItr < (_channel2values[ channel ]).end(); ){

					if ( (*posItr) == i ) posItr = (_channel2values[ channel ]).erase( posItr );
					else posItr++;
				}
			}
		}
		inline bool check( std::vector< std::string > channel, std::vector< std::vector< Token * > > setExpressions, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

			if ( _channel2values.count( channel ) > 0 ){

				//check everything in the database to see if any of it satisfies the set
				for ( auto dbValues = _channel2values.at( channel ).begin(); dbValues < _channel2values.at( channel ).end(); dbValues++ ){

					//require the same arity
					if (setExpressions.size() != (*dbValues).size()) continue;
		
					//check each value against its set expression
					bool allPassed = true;
					for ( unsigned int i = 0; i < (*dbValues).size(); i++ ){

						bool setEval = evalRPN_set( (*dbValues)[i], setExpressions[i], param2value, _globalVars, localVariables );						
						if (not setEval){
							allPassed = false;
							break;
						}
					}
					if (allPassed) return true;
				}
				return false; //if we made it all the way to the end and didn't find anything that satisfies the condition, return false
			}
			else return false;
		}
		inline std::vector< std::vector< int > > findAll( std::vector< std::string > channel, std::vector< std::vector< Token * > > setExpressions, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

			std::vector< std::vector< int > > out;

			if ( _channel2values.count( channel ) > 0 ){

				//check everything in the database to see if any of it satisfies the set
				for ( auto dbValues = _channel2values.at( channel ).begin(); dbValues < _channel2values.at( channel ).end(); dbValues++ ){

					//require the same arity
					if (setExpressions.size() != (*dbValues).size()) continue;
		
					//check each value against its set expression
					bool allPassed = true;
					for ( unsigned int i = 0; i < (*dbValues).size(); i++ ){

						bool setEval = evalRPN_set( (*dbValues)[i], setExpressions[i], param2value, _globalVars, localVariables );
						if (not setEval){
							allPassed = false;
							break;
						}
					}
					if (allPassed) out.push_back(*dbValues);
				}
			}
			return out;
		}
		void printContents(std::vector< std::string > channel){ //for testing

			std::cout << ">>>>>>>>>>>>DATABASE CONTENTS: ";
			if ( _channel2values.count( channel ) > 0 ){

				for ( auto dbValues = _channel2values.at( channel ).begin(); dbValues < _channel2values.at( channel ).end(); dbValues++ ){	

					for ( unsigned int i = 0; i < (*dbValues).size(); i++ ) std::cout << (*dbValues)[i] << " ";
				}
				std::cout << std::endl;
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
};



#endif
