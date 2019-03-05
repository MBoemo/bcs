//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#include "beacon.h"

BeaconChannel::BeaconChannel( std::string name, GlobalVariables &globalVars ){

	_channelName = name;
	_globalVars = globalVars;
}


std::string BeaconChannel::getChannelName(void){ return _channelName;}


void BeaconChannel::addCandidate( Block *b, SystemProcess *sp, std::list< SystemProcess > parallelProcesses, ParameterValues currentParameters, int &candidatesLeft, double &rateSum ){

	if ( b -> identify() == "MessageReceive" ){

		MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >( b );

		if ( mrb -> isCheck() ){

			//build the candidate
			std::shared_ptr<Candidate> cand( new Candidate( mrb, currentParameters, sp -> localVariables, sp, parallelProcesses) );
			double rate = evalRPN_double( b -> getRate(), currentParameters, _globalVars, sp -> localVariables );
			if ( rate <= 0 ) throw BadRate( b -> getToken() );
			cand -> rate = rate;
			std::set<int> setEval = evalRPN_set( mrb -> getSetExpression(), currentParameters, _globalVars, sp -> localVariables );
			cand -> rangeEvaluation = setEval;

			//check against the database
			bool canReceive = false;
			for ( auto i = setEval.begin(); i != setEval.end(); i++){

				if ( _database.check( _channelName, *i) ){
	
					canReceive = true;
					break;
				}
			}

			if ( not canReceive ){

				_activeBeaconReceiveCands[sp].push_back( cand );
				candidatesLeft++;
				rateSum += rate;
			}
			else {

				_potentialBeaconReceiveCands[sp].push_back( cand );
			}

		}
		else if ( not mrb -> isHandshake() ){ //beacon receive

			std::set<int> setEval = evalRPN_set( mrb -> getSetExpression(), currentParameters, _globalVars, sp -> localVariables );
			for ( auto i = setEval.begin(); i != setEval.end(); i++ ){

				std::map< std::string, double > augmentedLocalVars = sp -> localVariables;

				//if we have a binding variable, we're allowed to use it in the rate evaluation
				if ( mrb -> bindsVariable() ){

					augmentedLocalVars[ mrb -> getBindingVariable() ] = *i;
				}

				double rate = evalRPN_double( mrb -> getRate(), currentParameters, _globalVars, augmentedLocalVars );
				if ( rate <= 0 ) throw BadRate( b -> getToken() );
				std::shared_ptr<Candidate> cand( new Candidate(mrb, currentParameters, augmentedLocalVars, sp, parallelProcesses) );
				cand -> rate = rate;
				(cand -> rangeEvaluation).insert( *i );

				if ( _database.check( _channelName, *i) ){

					_activeBeaconReceiveCands[sp].push_back( cand );
					candidatesLeft++;
					rateSum += rate;
				}
				else{

					_potentialBeaconReceiveCands[sp].push_back( cand );
				}
			}
		}
		else assert(false);
	}
	else if ( b -> identify() == "MessageSend" ){

		MessageSendBlock *msb = dynamic_cast< MessageSendBlock * >( b );
		double rate = evalRPN_double( msb -> getRate(), currentParameters, _globalVars, sp -> localVariables );
		if ( rate <= 0 ) throw BadRate( b -> getToken() );

		std::shared_ptr< Candidate > cand( new Candidate( msb, currentParameters, sp -> localVariables, sp, parallelProcesses) );

		cand -> rate = rate;
		int paramEval = evalRPN_int( msb -> getParameterExpression(), currentParameters, _globalVars, sp -> localVariables );
		(cand -> rangeEvaluation).insert( paramEval );

		_sendCands[sp].push_back( cand );
		candidatesLeft++;
		rateSum += rate;
	}
	else assert(false);
}


void BeaconChannel::cleanSPFromChannel( SystemProcess *sp, int &candidatesLeft, double &rateSum ){

	//erase from potential receives
	if ( _potentialBeaconReceiveCands.find(sp) != _potentialBeaconReceiveCands.end() ){

		_potentialBeaconReceiveCands.erase(_potentialBeaconReceiveCands.find(sp));
	}

	//erase from active receives
	if ( _activeBeaconReceiveCands.find(sp) != _activeBeaconReceiveCands.end() ){

		for ( auto cand = _activeBeaconReceiveCands[sp].begin(); cand != _activeBeaconReceiveCands[sp].end(); cand++ ){

			candidatesLeft--;
			rateSum -= (*cand) -> rate;
		}
		_activeBeaconReceiveCands.erase( _activeBeaconReceiveCands.find(sp) );
	}

	//erase from sends
	if ( _sendCands.find(sp) != _sendCands.end() ){

		for ( auto cand = _sendCands[sp].begin(); cand != _sendCands[sp].end(); cand++ ){

			candidatesLeft--;
			rateSum -= (*cand) -> rate;
		}
		_sendCands.erase( _sendCands.find(sp) );
	}
}


void BeaconChannel::updateBeaconCandidates(int &candidatesLeft, double &rateSum){

	//move any actives that have become inactive to potential
	for ( auto candPair = _activeBeaconReceiveCands.begin(); candPair != _activeBeaconReceiveCands.end(); candPair++ ){

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end(); cand++){

			bool canReceive = false;
			MessageReceiveBlock *msb = dynamic_cast< MessageReceiveBlock * >( (*cand) -> actionCandidate );
			for ( auto i = ((*cand) -> rangeEvaluation).begin(); i != ((*cand) -> rangeEvaluation).end(); i++ ){

				if ( _database.check( _channelName, *i) ){
			
					canReceive = true;
					break;
				}
			}
			if ( (not canReceive and not msb -> isCheck()) or (canReceive and msb -> isCheck()) ){

				candidatesLeft--;
				rateSum -= (*cand) -> rate;
				_potentialBeaconReceiveCands[candPair -> first].push_back(*cand);
				cand = (candPair -> second).erase(cand);
			}
		}
	}

	//move any actives that have become inactive to potential
	for ( auto candPair = _potentialBeaconReceiveCands.begin(); candPair != _potentialBeaconReceiveCands.end(); candPair++ ){

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end(); cand++ ){

			bool canReceive = false;
			MessageReceiveBlock *msb = dynamic_cast< MessageReceiveBlock * >( (*cand) -> actionCandidate );
			for ( auto i = ((*cand) -> rangeEvaluation).begin(); i != ((*cand) -> rangeEvaluation).end(); i++ ){

				if ( _database.check( _channelName, *i) ){
			
					canReceive = true;
					break;
				}
			}
			if ( (canReceive and not msb -> isCheck()) or (not canReceive and msb -> isCheck()) ){

				candidatesLeft++;
				rateSum += (*cand) -> rate;
				_activeBeaconReceiveCands[candPair -> first].push_back(*cand);
				cand = (candPair -> second).erase(cand);
			}
		}
	}
}


std::shared_ptr<Candidate> BeaconChannel::pickCandidate(double &runningTotal, double uniformDraw, double rateSum){

	for ( auto candPair = _activeBeaconReceiveCands.begin(); candPair != _activeBeaconReceiveCands.end(); candPair++ ){

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end(); cand++ ){

			double r = (*cand) -> rate;
			double lower = runningTotal / rateSum;
			double upper = (runningTotal + r) / rateSum;

			if ( uniformDraw > lower and uniformDraw <= upper ){

				return *cand;
			}
			runningTotal += r;
		}
	}

	for ( auto candPair = _sendCands.begin(); candPair != _sendCands.end(); candPair++ ){

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end(); cand++ ){

			double r = (*cand) -> rate;
			double lower = runningTotal / rateSum;
			double upper = (runningTotal + r) / rateSum;

			if ( uniformDraw > lower and uniformDraw <= upper ){

				//update the database for the send or kill that we chose
				MessageSendBlock *msb = dynamic_cast< MessageSendBlock * >( (*cand) -> actionCandidate );
				if ( msb -> isKill() ){

					int param = evalRPN_int( msb -> getParameterExpression(), (*cand) -> parameterValues, _globalVars, (*cand) -> localVariables);
					_database.pop( _channelName, param );
				}
				else if ( not msb -> isHandshake() ){

					int param = evalRPN_int( msb -> getParameterExpression(), (*cand) -> parameterValues, _globalVars, (*cand) -> localVariables);
					_database.push( _channelName, param );
				}				
				return *cand;
			}
			runningTotal += r;
		}
	}
	return NULL;
}
