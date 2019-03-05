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
		std::map< std::string, std::vector< int > > _channel2values;
	public:
		inline void push( std::string channel, int i ){

			if ( _channel2values.count( channel ) > 0 ){

				if ( std::find( (_channel2values[ channel ]).begin(), (_channel2values[ channel ]).end(), i ) == (_channel2values[ channel ]).end() ){

					(_channel2values[ channel ]).push_back( i );
				}
			}
			else{

				_channel2values[ channel ] = { i };
			}
		}
		inline void pop( std::string channel, int i ){

			if ( _channel2values.count( channel ) > 0 ){

				for ( auto posItr = (_channel2values[ channel ]).begin(); posItr < (_channel2values[ channel ]).end(); posItr++ ){

					if ( (*posItr) == i ) (_channel2values[ channel ]).erase( posItr );
				}
			}
		}
		inline bool check( std::string channel, int i ){

			if ( _channel2values.count( channel ) > 0 ){

				if ( std::find( (_channel2values[ channel ]).begin(), (_channel2values[ channel ]).end(), i ) != (_channel2values[ channel ]).end() ){

					return true;
				}
				else return false;
			}
			else return false;
		}
};


class BeaconChannel{

	private:
		std::string _channelName;
		communicationDatabase _database;
		GlobalVariables _globalVars;
		std::map< SystemProcess *, std::list< std::shared_ptr<Candidate> > > _potentialBeaconReceiveCands;
		std::map< SystemProcess *, std::list< std::shared_ptr<Candidate> > > _activeBeaconReceiveCands;
		std::map< SystemProcess *, std::list< std::shared_ptr<Candidate> > > _sendCands;

	public:
		BeaconChannel( std::string, GlobalVariables & );
		BeaconChannel( const BeaconChannel & );
		std::string getChannelName(void);
		void updateBeaconCandidates(int &, double &);
		void cleanSPFromChannel( SystemProcess *, int &, double & );
		std::shared_ptr<Candidate> pickCandidate(double &, double , double );
		void addCandidate( Block *, SystemProcess *, std::list< SystemProcess > , ParameterValues, int &, double & );
};



#endif
