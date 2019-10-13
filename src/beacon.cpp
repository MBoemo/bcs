//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG 1

#include "beacon.h"

BeaconChannel::BeaconChannel( std::vector< std::string > name, GlobalVariables &globalVars ){

	_channelName = name;
	_globalVars = globalVars;
}


std::vector< std::string > BeaconChannel::getChannelName(void){ return _channelName;}


void BeaconChannel::addCandidate( Block *b, SystemProcess *sp, std::list< SystemProcess > parallelProcesses, ParameterValues &currentParameters, int &candidatesLeft, double &rateSum ){
//returns a bool of whether the candidate was added (if false, it has been added to potential receives)

#if DEBUG
std::cout << std::endl;
_database.printContents(_channelName);
#endif

	if ( b -> identify() == "MessageReceive" ){

		MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >( b );

		if ( mrb -> isCheck() ){

			//build the candidate
			std::shared_ptr<Candidate> cand( new Candidate( mrb, currentParameters, sp -> localVariables, sp, parallelProcesses) );
			Numerical rate = evalRPN_numerical( b -> getRate(), currentParameters, _globalVars, sp -> localVariables );
			if ( rate.doubleCast() <= 0 ) throw BadRate( b -> getToken() );
			cand -> rate = rate.doubleCast();

			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			bool canReceive = _database.check( _channelName, setExpressions, currentParameters, _globalVars, sp -> localVariables );

			if ( not canReceive ){//only do a beacon check if you can't receive

				_activeBeaconReceiveCands[sp].push_back( cand );
				candidatesLeft++;
				rateSum += rate.doubleCast();
			}
			else _potentialBeaconReceiveCands[sp].push_back( cand );
	
#if DEBUG
std::cout << ">>>>>>>>>>>>Adding candidate: Beacon check ";
Token *t = b -> getToken();
std::cout << t -> value() << std::endl;
std::cout << ">>>>>>>>>>>>Can receive? " << canReceive << std::endl;
#endif

		}
		else if ( not mrb -> isHandshake() ){ //beacon receive

			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			std::vector< std::vector< int > > matchingParameters = _database.findAll( _channelName, setExpressions, currentParameters, _globalVars, sp -> localVariables );

			//build a candidate for each possible beacon receive on this parameter set
			for ( auto mp = matchingParameters.begin(); mp < matchingParameters.end(); mp++ ){

				//if we have binding variables, we're allowed to use it in the rate evaluation
				std::map< std::string, Numerical > augmentedLocalVars = sp -> localVariables;
				std::vector<Numerical> newRangeEval;
				if ( mrb -> bindsVariable() ){

					std::vector< std::string > bindingVarNames = mrb -> getBindingVariable();
					for ( unsigned int i = 0; i < bindingVarNames.size(); i++ ){

						Numerical n;
						n.setInt((*mp)[i]);
						newRangeEval.push_back(n);
						augmentedLocalVars[ bindingVarNames[i] ] = n;
					}
				}

				Numerical rate = evalRPN_numerical( mrb -> getRate(), currentParameters, _globalVars, augmentedLocalVars );
				if ( rate.doubleCast() <= 0 ) throw BadRate( b -> getToken() );
				std::shared_ptr<Candidate> cand( new Candidate(mrb, currentParameters, augmentedLocalVars, sp, parallelProcesses) );
				cand -> rate = rate.doubleCast();
				cand -> rangeEvaluation = newRangeEval;
				_activeBeaconReceiveCands[sp].push_back( cand );
				candidatesLeft++;
				rateSum += rate.doubleCast();
			}

			//if the mrb can't receive and isn't already in the potential receives, add it to the potential receives
			if ( matchingParameters.size() == 0){

				std::shared_ptr<Candidate> cand( new Candidate(mrb, currentParameters, sp -> localVariables, sp, parallelProcesses) );
				_potentialBeaconReceiveCands[sp].push_back( cand );
			}

#if DEBUG
std::cout << ">>>>>>>>>>>>Adding candidate: Beacon receive ";
Token *t = b -> getToken();
std::cout << t -> value() << std::endl;
std::cout << ">>>>>>>>>>>>Can receive? " << matchingParameters.size() << std::endl;
#endif
		}
		else assert(false);
	}
	else{

#if DEBUG
std::cout << ">>>>>>>>>>>>Adding candidate: Beacon send ";
Token *t = b -> getToken();
std::cout << t -> value() << std::endl;
#endif

		assert( b -> identify() == "MessageSend" );
		MessageSendBlock *msb = dynamic_cast< MessageSendBlock * >( b );
		Numerical rate = evalRPN_numerical( msb -> getRate(), currentParameters, _globalVars, sp -> localVariables );
		if ( rate.doubleCast() <= 0 ) throw BadRate( b -> getToken() );

		std::shared_ptr< Candidate > cand( new Candidate( msb, currentParameters, sp -> localVariables, sp, parallelProcesses) );

		cand -> rate = rate.doubleCast();

		//evaluate the expression
		std::vector< std::vector< Token * > > parameterExpressions = msb -> getParameterExpression();
		std::vector<Numerical> param;
		for ( auto exp = parameterExpressions.begin(); exp < parameterExpressions.end(); exp++ ){

			Numerical paramEval = evalRPN_numerical( *exp, currentParameters, _globalVars, sp -> localVariables );
			param.push_back(paramEval);
		}
		cand -> rangeEvaluation = param;

		_sendCands[sp].push_back( cand );
		candidatesLeft++;
		rateSum += rate.doubleCast();
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


void BeaconChannel::updateBeaconCandidates(int &candidatesLeft, double &rateSum){

	//move any actives that have become inactive to potential
	for ( auto candPair = _activeBeaconReceiveCands.begin(); candPair != _activeBeaconReceiveCands.end(); candPair++ ){

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end();){

			MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >( (*cand) -> actionCandidate );
			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			SystemProcess *sp = (*cand) -> processInSystem;

#if DEBUG
_database.printContents(_channelName);
#endif

			bool canReceive = _database.check( _channelName, setExpressions, sp -> parameterValues, _globalVars, sp -> localVariables );

			if ( (not canReceive and not mrb -> isCheck()) or (canReceive and mrb -> isCheck()) ){

				candidatesLeft--;
				rateSum -= (*cand) -> rate;
				_potentialBeaconReceiveCands[candPair -> first].push_back(*cand);
				cand = (candPair -> second).erase(cand);
			}
			else cand++;
		}
	}

	//move any potentials to active if they can now receive
	for ( auto candPair = _potentialBeaconReceiveCands.begin(); candPair != _potentialBeaconReceiveCands.end(); candPair++ ){

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end();){

			SystemProcess *sp = (*cand) -> processInSystem;
			MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >( (*cand) -> actionCandidate );
			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			bool canReceive = _database.check( _channelName, setExpressions, sp -> parameterValues, _globalVars, sp -> localVariables );
			if (mrb -> isCheck() and not canReceive){

				_activeBeaconReceiveCands[sp].push_back(*cand);
				Numerical rate = evalRPN_numerical( mrb -> getRate(), sp -> parameterValues, _globalVars, sp -> localVariables );
				if ( rate.doubleCast() <= 0 ) throw BadRate( mrb -> getToken() );
				candidatesLeft++;
				rateSum += rate.doubleCast();
				cand = (candPair -> second).erase(cand);			
			}
			else if (not mrb -> isCheck()){

				std::vector< std::vector< int > > matchingParameters = _database.findAll( _channelName, setExpressions, sp -> parameterValues, _globalVars, sp -> localVariables );

				//build a candidate for each possible beacon receive on this parameter set
				for ( auto mp = matchingParameters.begin(); mp < matchingParameters.end(); mp++ ){

					//if we have binding variables, we're allowed to use it in the rate evaluation
					std::map< std::string, Numerical > augmentedLocalVars = sp -> localVariables;
					std::vector<Numerical> newRangeEval;
					if ( mrb -> bindsVariable() ){

						std::vector< std::string > bindingVarNames = mrb -> getBindingVariable();
						for ( unsigned int i = 0; i < bindingVarNames.size(); i++ ){

							Numerical n;
							n.setInt((*mp)[i]);
							newRangeEval.push_back(n);
							augmentedLocalVars[ bindingVarNames[i] ] = n;
						}
					}

					Numerical rate = evalRPN_numerical( mrb -> getRate(), sp -> parameterValues, _globalVars, augmentedLocalVars );
					if ( rate.doubleCast() <= 0 ) throw BadRate( mrb -> getToken() );
					std::shared_ptr<Candidate> newCand( new Candidate(mrb, sp -> parameterValues, augmentedLocalVars, sp, (*cand) -> parallelProcesses) );
					newCand -> rate = rate.doubleCast();
					newCand -> rangeEvaluation = newRangeEval;
					_activeBeaconReceiveCands[sp].push_back( newCand );
					candidatesLeft++;
					rateSum += rate.doubleCast();
				}

				if (matchingParameters.size() > 0) cand = (candPair -> second).erase(cand);
				else cand++;
			}
			else cand++;
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

						Numerical paramEval = evalRPN_numerical( *exp, (*cand) -> parameterValues, _globalVars, sp -> localVariables );
						if (paramEval.isDouble()) throw WrongType((*exp)[0],"Parameter expressions in message receive must evaluate to ints, not doubles (either through explicit or implicit casting).");
						param.push_back(paramEval.getInt());
					}

					_database.pop( _channelName, param );
				}
				else if ( not msb -> isHandshake() ){

					//evaluate the expression
					std::vector<int> param;
					for ( auto exp = parameterExpressions.begin(); exp < parameterExpressions.end(); exp++ ){

						Numerical paramEval = evalRPN_numerical( *exp, (*cand) -> parameterValues, _globalVars, sp -> localVariables );
						if (paramEval.isDouble()) throw WrongType((*exp)[0],"Parameter expressions in message receive must evaluate to ints, not doubles (either through explicit or implicit casting).");
						param.push_back(paramEval.getInt());
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
