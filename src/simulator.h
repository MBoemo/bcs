 //----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <memory>
#include <chrono>
#include <list>
#include <iomanip>
#include <sstream>
#include <iterator>
#include "error_handling.h"
#include "handshake.h"
#include "beacon.h"

class System{

	private: 
		std::list< SystemProcess * > _currentProcesses;
		GlobalVariables _globalVars;
		double _rateSum = 0.0, _totalTime = 0.0, _maxDuration;
		int _transitionsTaken = 0, _maxTransitions, _candidatesLeft = 0;

		std::map< SystemProcess * , std::vector< std::shared_ptr<Candidate> > > _nonMsgCandidates;
		std::map< std::vector<std::string>, std::shared_ptr<BeaconChannel> > _beacons_Name2Channel;
		std::map< std::vector<std::string>, std::shared_ptr<HandshakeChannel> > _handshakes_Name2Channel;

		std::map< std::string, ProcessDefinition > _name2ProcessDef;
		std::stringstream _outputStream;

		void splitOnParallel( SystemProcess *, Block *, std::list< SystemProcess * > & );

	public:
		System( std::list< SystemProcess > &, std::map< std::string, ProcessDefinition > &, int, double , GlobalVariables & );
		~System(){

			for ( auto i = _currentProcesses.begin(); i != _currentProcesses.end(); i++ ){

				delete *i;
			}
		}
		void writeTransition( double , std::shared_ptr<Candidate>, std::stringstream & );
		std::string writeChannelName( std::vector< std::vector< Token * > > );
		std::vector< std::string > substituteChannelName( std::vector< std::vector< Token * > >, ParameterValues &, std::map< std::string, Numerical > & );
		void sumTransitionRates( SystemProcess *, Tree<Block> &, Block *, std::list< SystemProcess >, ParameterValues & );
		void updateSystem( std::shared_ptr<Candidate>, std::list< SystemProcess * > & );
		void splitOnParallel(SystemProcess &, Block *, std::list< SystemProcess> & );
		void simulate( void );
		std::streambuf *write( void ){ return _outputStream.rdbuf(); }
		void removeChosenFromSystem( std::shared_ptr<Candidate> );
		void getParallelProcesses( std::shared_ptr<Candidate>, std::list< SystemProcess * > & );
		SystemProcess * updateSpForTransition( std::shared_ptr<Candidate> );
		bool variableIsDefined(std::string, ParameterValues &, std::map< std::string, Numerical > &);
		void printTransition(double, std::shared_ptr<Candidate>);
};


class progressBar{

	private:
		std::chrono::time_point<std::chrono::steady_clock> _startTime;
		std::chrono::time_point<std::chrono::steady_clock> _currentTime;
		unsigned int maxNumber;
		unsigned int barWidth = 35;
		unsigned int _digits;

	public:
		progressBar( unsigned int maxNumber ){
		
			this -> maxNumber = maxNumber;
			_digits = std::to_string( maxNumber).length() + 1;
			_startTime = std::chrono::steady_clock::now();
		}
		void displayProgress( unsigned int currentNumber ){

			_currentTime = std::chrono::steady_clock::now();
			 std::chrono::duration<double> elapsedTime = _currentTime - _startTime;

			double progress = (double) currentNumber / (double) maxNumber;
			
			if ( progress <= 1.0 ){

				std::cout << "[";
				unsigned int pos = barWidth * progress;
				for (unsigned int i = 0; i < barWidth; ++i) {
					if (i < pos) std::cout << "=";
					else if (i == pos) std::cout << ">";
					else std::cout << " ";
				}
				std::cout << "] " << std::right << std::setw(3) << int(progress * 100.0) << "%  ";

				std::cout << std::right << std::setw(_digits) << currentNumber << "/" << maxNumber << "  ";


				unsigned int estTimeLeft = elapsedTime.count() * ( (double) maxNumber / (double) currentNumber - 1.0 );			
				unsigned int hours = estTimeLeft / 3600;
				unsigned int mins = (estTimeLeft % 3600) / 60;
				unsigned int secs = (estTimeLeft % 3600) % 60;

				std::cout << std::right << std::setw(2) << hours << "hr" << std::setw(2) << mins << "min" << std::setw(2) << secs << "sec  "<< "\r";
				std::cout.flush();
			}
		} 
};


void simulateSystem( std::map< std::string, ProcessDefinition > &, std::list< SystemProcess > &, GlobalVariables &, int, int, std::string, int, double );

#endif
