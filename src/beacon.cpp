//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#include "beacon.h"

BeaconChannel::BeaconChannel( std::vector< std::string > name, GlobalVariables &globalVars ){

	_channelName = name;
	_globalVars = globalVars;
}


std::vector< std::string > BeaconChannel::getChannelName(void){ return _channelName;}


void BeaconChannel::addCandidate( Block *b, SystemProcess *sp, std::list< SystemProcess > parallelProcesses, ParameterValues currentParameters, int &candidatesLeft, double &rateSum ){
//returns a bool of whether the candidate was added (if false, it has been added to potential receives)

	if ( b -> identify() == "MessageReceive" ){

		MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >( b );

		if ( mrb -> isCheck() ){

			//build the candidate
			std::shared_ptr<Candidate> cand( new Candidate( mrb, currentParameters, sp -> localVariables, sp, parallelProcesses) );
			double rate = evalRPN_double( b -> getRate(), currentParameters, _globalVars, sp -> localVariables );
			if ( rate <= 0 ) throw BadRate( b -> getToken() );
			cand -> rate = rate;

			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			bool canReceive = _database.check( _channelName, setExpressions, currentParameters, _globalVars, sp -> localVariables );

			if ( canReceive ){

				_activeBeaconReceiveCands[sp].push_back( cand );
				candidatesLeft++;
				rateSum += rate;
			}
			else {

				_potentialBeaconReceiveCands[sp].push_back( cand );
			}
		}
		else if ( not mrb -> isHandshake() ){ //beacon receive

			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			std::vector< std::vector< int > > matchingParameters = _database.findAll( _channelName, setExpressions, currentParameters, _globalVars, sp -> localVariables );

			//build a candidate for each possible beacon receive on this parameter set
			for ( auto mp = matchingParameters.begin(); mp < matchingParameters.end(); mp++ ){

				//if we have binding variables, we're allowed to use it in the rate evaluation
				std::map< std::string, double > augmentedLocalVars = sp -> localVariables;
				if ( mrb -> bindsVariable() ){

					std::vector< std::string > bindingVarNames = mrb -> getBindingVariable();
					for ( unsigned int i = 0; i < bindingVarNames.size(); i++ ){

						augmentedLocalVars[ bindingVarNames[i] ] = (*mp)[i];
					}
				}

				double rate = evalRPN_double( mrb -> getRate(), currentParameters, _globalVars, augmentedLocalVars );
				if ( rate <= 0 ) throw BadRate( b -> getToken() );
				std::shared_ptr<Candidate> cand( new Candidate(mrb, currentParameters, augmentedLocalVars, sp, parallelProcesses) );
				cand -> rate = rate;
				cand -> rangeEvaluation = *mp;
				_activeBeaconReceiveCands[sp].push_back( cand );
				candidatesLeft++;
				rateSum += rate;
			}

			//if the mrb can't receive and isn't already in the potential receives, add it to the potential receives
			if ( matchingParameters.size() == 0){

				std::shared_ptr<Candidate> cand( new Candidate(mrb, currentParameters, sp -> localVariables, sp, parallelProcesses) );
				_potentialBeaconReceiveCands[sp].push_back( cand );
			}
		}
		else assert(false);
	}
	else{
		assert( b -> identify() == "MessageSend" );
		MessageSendBlock *msb = dynamic_cast< MessageSendBlock * >( b );
		double rate = evalRPN_double( msb -> getRate(), currentParameters, _globalVars, sp -> localVariables );
		if ( rate <= 0 ) throw BadRate( b -> getToken() );

		std::shared_ptr< Candidate > cand( new Candidate( msb, currentParameters, sp -> localVariables, sp, parallelProcesses) );

		cand -> rate = rate;

		//evaluate the expression
		std::vector< std::vector< Token * > > parameterExpressions = msb -> getParameterExpression();
		std::vector<int> param;
		for ( auto exp = parameterExpressions.begin(); exp < parameterExpressions.end(); exp++ ){

			int paramEval = evalRPN_int( *exp, currentParameters, _globalVars, sp -> localVariables );
			param.push_back(paramEval);
		}
		cand -> rangeEvaluation = param;

		_sendCands[sp].push_back( cand );
		candidatesLeft++;
		rateSum += rate;
	}
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


void BeaconChannel::updateBeaconCandidates(int &candidatesLeft, double &rateSum, ParameterValues currentParameters ){


	//move any actives that have become inactive to potential
	for ( auto candPair = _activeBeaconReceiveCands.begin(); candPair != _activeBeaconReceiveCands.end(); candPair++ ){

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end(); cand++){

			MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >( (*cand) -> actionCandidate );
			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			SystemProcess *sp = (*cand) -> processInSystem;
			bool canReceive = _database.check( _channelName, setExpressions, currentParameters, _globalVars, sp -> localVariables );
			if ( (not canReceive and not mrb -> isCheck()) or (canReceive and mrb -> isCheck()) ){

				candidatesLeft--;
				rateSum -= (*cand) -> rate;
				_potentialBeaconReceiveCands[candPair -> first].push_back(*cand);
				cand = (candPair -> second).erase(cand);
			}
		}
	}

	//move any potentials to active if they can now receive
	for ( auto candPair = _potentialBeaconReceiveCands.begin(); candPair != _potentialBeaconReceiveCands.end(); candPair++ ){

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end(); cand++ ){

			SystemProcess *sp = (*cand) -> processInSystem;
			MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >( (*cand) -> actionCandidate );
			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			std::vector< std::vector< int > > matchingParameters = _database.findAll( _channelName, setExpressions, currentParameters, _globalVars, sp -> localVariables );

			//build a candidate for each possible beacon receive on this parameter set
			for ( auto mp = matchingParameters.begin(); mp < matchingParameters.end(); mp++ ){

				//if we have binding variables, we're allowed to use it in the rate evaluation
				std::map< std::string, double > augmentedLocalVars = sp -> localVariables;
				if ( mrb -> bindsVariable() ){

					std::vector< std::string > bindingVarNames = mrb -> getBindingVariable();
					for ( unsigned int i = 0; i < bindingVarNames.size(); i++ ){

						augmentedLocalVars[ bindingVarNames[i] ] = (*mp)[i];
					}
				}

				double rate = evalRPN_double( mrb -> getRate(), currentParameters, _globalVars, augmentedLocalVars );
				if ( rate <= 0 ) throw BadRate( mrb -> getToken() );
				std::shared_ptr<Candidate> newCand( new Candidate(mrb, currentParameters, augmentedLocalVars, sp, (*cand) -> parallelProcesses) );
				newCand -> rate = rate;
				newCand -> rangeEvaluation = *mp;
				_activeBeaconReceiveCands[sp].push_back( newCand );
				candidatesLeft++;
				rateSum += rate;
			}
			//if we swapped a candidate from potential to active, delete it from potential
			if ( matchingParameters.size() > 0) cand = (candPair -> second).erase(cand);
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
				SystemProcess *sp = (*cand) -> processInSystem;
				std::vector< std::vector< Token * > > parameterExpressions = msb -> getParameterExpression();
				if ( msb -> isKill() ){

					//evaluate the expression
					std::vector<int> param;
					for ( auto exp = parameterExpressions.begin(); exp < parameterExpressions.end(); exp++ ){

						int paramEval = evalRPN_int( *exp, (*cand) -> parameterValues, _globalVars, sp -> localVariables );
						param.push_back(paramEval);
					}

					_database.pop( _channelName, param );
				}
				else if ( not msb -> isHandshake() ){

					//evaluate the expression
					std::vector<int> param;
					for ( auto exp = parameterExpressions.begin(); exp < parameterExpressions.end(); exp++ ){

						int paramEval = evalRPN_int( *exp, (*cand) -> parameterValues, _globalVars, sp -> localVariables );
						param.push_back(paramEval);
					}
					_database.push( _channelName, param );
				}				
				return *cand;
			}
			runningTotal += r;
		}
	}
	return NULL;
}
