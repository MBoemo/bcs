//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG 1

#include "beacon.h"
#include "common.h"
#include <algorithm>

BeaconChannel::BeaconChannel( std::vector< std::string > name, GlobalVariables &globalVars ){

	_channelName = name;
	_globalVars = globalVars;
}


std::vector< std::string > BeaconChannel::getChannelName(void){ return _channelName;}


void BeaconChannel::addCandidate( Block *b, SystemProcess *sp, std::list< SystemProcess > parallelProcesses, ParameterValues &currentParameters, int &candidatesLeft, double &rateSum ){
//returns a bool of whether the candidate was added (if false, it has been added to potential receives)

#if DEBUG
std::cout << std::endl;
_database.printContents();
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
			bool canReceive;
			if (mrb -> usesSets()){
				canReceive = _database.check( setExpressions, currentParameters, _globalVars, sp -> localVariables );
			}
			else{
				canReceive = _database.check_quick( setExpressions, currentParameters, _globalVars, sp -> localVariables );
			}

			if ( not canReceive ){//only do a beacon check if you can't receive

				_activeBeaconReceiveCands[sp].push_back( cand );
				candidatesLeft += sp -> clones;
				rateSum += rate.doubleCast() * (sp -> clones);
			}
			else _potentialBeaconReceiveCands[sp].push_back( cand );

#if DEBUG
std::cout << "   >>Adding candidate: Beacon check ";
Token *t = b -> getToken();
std::cout << t -> value() << std::endl;
std::cout << "   >>Can receive? " << canReceive << std::endl;
#endif

		}
		else if ( not mrb -> isHandshake() ){ //beacon receive

			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			std::vector< std::vector< int > > matchingParameters;
			if (mrb -> usesSets()){
				matchingParameters = _database.findAll( setExpressions, currentParameters, _globalVars, sp -> localVariables );
			}
			else{
				matchingParameters = _database.findAll_trivial( setExpressions, currentParameters, _globalVars, sp -> localVariables );
			}

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
				candidatesLeft += sp -> clones;
				rateSum += rate.doubleCast() * (sp -> clones);
			}

			//if the mrb can't receive and isn't already in the potential receives, add it to the potential receives
			if ( matchingParameters.size() == 0){

				std::shared_ptr<Candidate> cand( new Candidate(mrb, currentParameters, sp -> localVariables, sp, parallelProcesses) );
				_potentialBeaconReceiveCands[sp].push_back( cand );
			}

#if DEBUG
std::cout << "   >>Adding candidate: Beacon receive ";
Token *t = b -> getToken();
std::cout << t -> value() << std::endl;
std::cout << "   >>Can receive? " << matchingParameters.size() << std::endl;
#endif
		}
		else assert(false);
	}
	else{

#if DEBUG
std::cout << "   >>Adding candidate: Beacon send ";
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
		candidatesLeft += sp -> clones;
		rateSum += rate.doubleCast() * (sp -> clones);
	}
}


void BeaconChannel::cleanSPFromChannel( SystemProcess *sp, int &candidatesLeft, double &rateSum ){

#if DEBUG
std::cout << "   >>Cleaning " << sp << " from channel - before clean" << std::endl;
std::cout << "   >>clones: " << sp -> clones << std::endl;
for (auto a = _potentialBeaconReceiveCands.begin(); a != _potentialBeaconReceiveCands.end(); a++) std::cout << "   >>Potential receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _activeBeaconReceiveCands.begin(); a != _activeBeaconReceiveCands.end(); a++) std::cout << "   >>Active receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _sendCands.begin(); a != _sendCands.end(); a++) std::cout << "   >>Active sends: " << a -> first << " " << (a -> second).size() << std::endl;
#endif

	//erase from potential receives
	if ( _potentialBeaconReceiveCands.find(sp) != _potentialBeaconReceiveCands.end() and sp -> clones == 1){

		_potentialBeaconReceiveCands.erase(_potentialBeaconReceiveCands.find(sp));
	}

	//erase from active receives
	if ( _activeBeaconReceiveCands.find(sp) != _activeBeaconReceiveCands.end() ){

		for ( auto cand = _activeBeaconReceiveCands[sp].begin(); cand != _activeBeaconReceiveCands[sp].end(); cand++ ){

			candidatesLeft--;
			rateSum -= (*cand) -> rate;
		}
		if (sp -> clones == 1) _activeBeaconReceiveCands.erase( _activeBeaconReceiveCands.find(sp) );
	}

	//erase from sends
	if ( _sendCands.find(sp) != _sendCands.end() ){

		for ( auto cand = _sendCands[sp].begin(); cand != _sendCands[sp].end(); cand++ ){

			candidatesLeft--;
			rateSum -= (*cand) -> rate;
		}
		if (sp -> clones == 1) _sendCands.erase( _sendCands.find(sp) );
	}
#if DEBUG
std::cout << "   >>Cleaning " << sp << " from channel - after clean" << std::endl;
std::cout << "   >>clones: " << sp -> clones << std::endl;
for (auto a = _potentialBeaconReceiveCands.begin(); a != _potentialBeaconReceiveCands.end(); a++) std::cout << "   >>Potential receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _activeBeaconReceiveCands.begin(); a != _activeBeaconReceiveCands.end(); a++) std::cout << "   >>Active receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _sendCands.begin(); a != _sendCands.end(); a++) std::cout << "   >>Active sends: " << a -> first << " " << (a -> second).size() << std::endl;
#endif
}


void BeaconChannel::updateBeaconCandidates(int &candidatesLeft, double &rateSum){

#if DEBUG
std::cout << "   >>Updating candidates - moving from active to potential...." << std::endl;
_database.printContents();
for (auto a = _potentialBeaconReceiveCands.begin(); a != _potentialBeaconReceiveCands.end(); a++) std::cout << "   >>Potential receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _activeBeaconReceiveCands.begin(); a != _activeBeaconReceiveCands.end(); a++) std::cout << "   >>Active receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _sendCands.begin(); a != _sendCands.end(); a++) std::cout << "   >>Active sends: " << a -> first << " " << (a -> second).size() << std::endl;
#endif

	//move any active beacons that have become inactive to potential
	for ( auto candPair = _activeBeaconReceiveCands.begin(); candPair != _activeBeaconReceiveCands.end(); candPair++ ){

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end();){

			MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >( (*cand) -> actionCandidate );
			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();
			SystemProcess *sp = (*cand) -> processInSystem;
			assert(sp == candPair -> first);

			bool canReceive;
			if (mrb -> usesSets()){
				canReceive = _database.check( setExpressions, sp -> parameterValues, _globalVars, sp -> localVariables );
			}
			else{
				canReceive = _database.check_quick( setExpressions, sp -> parameterValues, _globalVars, sp -> localVariables );
			}

			if ( (not canReceive and not mrb -> isCheck()) or (canReceive and mrb -> isCheck()) ){

				candidatesLeft -= sp -> clones;
				rateSum -= (*cand) -> rate * (sp -> clones);
				_potentialBeaconReceiveCands[candPair -> first].push_back(*cand);
				cand = (candPair -> second).erase(cand);
			}
			else cand++;
		}
	}

#if DEBUG
std::cout << "   >>Updating candidates - moving from potential to active...." << std::endl;
_database.printContents();
for (auto a = _potentialBeaconReceiveCands.begin(); a != _potentialBeaconReceiveCands.end(); a++) std::cout << "   >>Potential receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _activeBeaconReceiveCands.begin(); a != _activeBeaconReceiveCands.end(); a++) std::cout << "   >>Active receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _sendCands.begin(); a != _sendCands.end(); a++) std::cout << "   >>Active sends: " << a -> first << " " << (a -> second).size() << std::endl;
#endif

	//move any potentials to active if they can now receive
	for ( auto candPair = _potentialBeaconReceiveCands.begin(); candPair != _potentialBeaconReceiveCands.end(); candPair++ ){


		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end();){

			SystemProcess *sp = (*cand) -> processInSystem;
			assert(sp == candPair -> first);
			MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >( (*cand) -> actionCandidate );
			std::vector< std::vector< Token * > > setExpressions = mrb -> getSetExpression();

			bool canReceive;
			if (mrb -> usesSets()){
				canReceive = _database.check( setExpressions, sp -> parameterValues, _globalVars, sp -> localVariables );
			}
			else{
				canReceive = _database.check_quick( setExpressions, sp -> parameterValues, _globalVars, sp -> localVariables );
			}

			if (mrb -> isCheck() and not canReceive){

				_activeBeaconReceiveCands[sp].push_back(*cand);
				Numerical rate = evalRPN_numerical( mrb -> getRate(), sp -> parameterValues, _globalVars, sp -> localVariables );
				if ( rate.doubleCast() <= 0 ) throw BadRate( mrb -> getToken() );
				candidatesLeft += sp -> clones;
				rateSum += rate.doubleCast() * (sp -> clones);
				cand = (candPair -> second).erase(cand);
			}
			else if (not mrb -> isCheck()){

				std::vector< std::vector< int > > matchingParameters;
				if (mrb -> usesSets()){
					matchingParameters = _database.findAll( setExpressions, sp -> parameterValues, _globalVars, sp -> localVariables );
				}
				else{
					matchingParameters = _database.findAll_trivial( setExpressions, sp -> parameterValues, _globalVars, sp -> localVariables );
				}

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
					candidatesLeft += sp -> clones;
					rateSum += rate.doubleCast() * (sp -> clones);
				}
				if (matchingParameters.size() > 0) cand = (candPair -> second).erase(cand);
				else cand++;
			}
			else cand++;
		}
	}

#if DEBUG
std::cout << "   >>Updating candidates - should be finished...." << std::endl;
_database.printContents();
for (auto a = _potentialBeaconReceiveCands.begin(); a != _potentialBeaconReceiveCands.end(); a++) std::cout << "   >>Potential receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _activeBeaconReceiveCands.begin(); a != _activeBeaconReceiveCands.end(); a++) std::cout << "   >>Active receives: " << a -> first << " " << (a -> second).size() << std::endl;
for (auto a = _sendCands.begin(); a != _sendCands.end(); a++) std::cout << "   >>Active sends: " << a -> first << " " << (a -> second).size() << std::endl;
#endif
}


std::shared_ptr<Candidate> BeaconChannel::pickCandidate(double &runningTotal, double uniformDraw, double rateSum){

#if DEBUG
std::cout << "   >>Picking beacon candidate..." << std::endl;
#endif

	for ( auto candPair = _activeBeaconReceiveCands.begin(); candPair != _activeBeaconReceiveCands.end(); candPair++ ){

		SystemProcess *sp = candPair -> first;

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end(); cand++ ){

			double r = (*cand) -> rate * (sp -> clones);
			double lower = runningTotal / rateSum;
			double upper = (runningTotal + r) / rateSum;

			if ( uniformDraw > lower and uniformDraw <= upper ){

				return *cand;
			}
			runningTotal += r;
		}
	}

	for ( auto candPair = _sendCands.begin(); candPair != _sendCands.end(); candPair++ ){

		SystemProcess *sp = candPair -> first;

		for ( auto cand = (candPair -> second).begin(); cand != (candPair -> second).end(); cand++ ){

			double r = (*cand) -> rate * (sp -> clones);
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

					_database.pop( param );
				}
				else if ( not msb -> isHandshake() ){

					//evaluate the expression
					std::vector<int> param;
					for ( auto exp = parameterExpressions.begin(); exp < parameterExpressions.end(); exp++ ){

						Numerical paramEval = evalRPN_numerical( *exp, (*cand) -> parameterValues, _globalVars, sp -> localVariables );
						if (paramEval.isDouble()) throw WrongType((*exp)[0],"Parameter expressions in message receive must evaluate to ints, not doubles (either through explicit or implicit casting).");
						param.push_back(paramEval.getInt());
					}
					_database.push( param );
				}				
				return *cand;
			}
			runningTotal += r;
		}
	}
	return NULL;
}


bool BeaconChannel::matchClone( SystemProcess *newSp, SystemProcess *existingSp){

#if DEBUG
std::cout << "Matching clones...." << std::endl;
std::cout << "System Processes: " << existingSp << " " << newSp << std::endl;
std::cout << "Potential Beacon Receives: " << _potentialBeaconReceiveCands.count(existingSp) << " " << _potentialBeaconReceiveCands.count(newSp) << std::endl;
std::cout << "Active Beacon Receives: " << _activeBeaconReceiveCands.count(existingSp) << " " << _activeBeaconReceiveCands.count(newSp) << std::endl;
std::cout << "Beacon Sends: " << _sendCands.count(existingSp) << " " << _sendCands.count(newSp) << std::endl;
#endif

	if ( _potentialBeaconReceiveCands[existingSp].size() == 0 && _potentialBeaconReceiveCands[newSp].size() == 0
	  && _activeBeaconReceiveCands[existingSp].size() == 0 && _activeBeaconReceiveCands[newSp].size() == 0
	  && _sendCands[existingSp].size() == 0 && _sendCands[newSp].size() == 0) return true;

	assert(_potentialBeaconReceiveCands[existingSp].size() == _potentialBeaconReceiveCands[newSp].size());
	assert(_activeBeaconReceiveCands[existingSp].size() == _activeBeaconReceiveCands[newSp].size());
	assert(_sendCands[existingSp].size() == _sendCands[newSp].size());

	bool matchPotReceives = std::is_permutation(_potentialBeaconReceiveCands[newSp].begin(), _potentialBeaconReceiveCands[newSp].end(), _potentialBeaconReceiveCands[existingSp].begin(), compareCandidates);
	bool matchActReceives = std::is_permutation(_activeBeaconReceiveCands[newSp].begin(), _activeBeaconReceiveCands[newSp].end(), _activeBeaconReceiveCands[existingSp].begin(), compareCandidates);
	bool matchSends = std::is_permutation(_sendCands[newSp].begin(), _sendCands[newSp].end(), _sendCands[existingSp].begin(), compareCandidates);

	if (matchPotReceives and matchActReceives and matchSends) return true;
	else return false;
}


void BeaconChannel::cleanCloneFromChannel( SystemProcess *sp ){

	auto prLoc = _potentialBeaconReceiveCands.find( sp );
	if (prLoc != _potentialBeaconReceiveCands.end()) _potentialBeaconReceiveCands.erase( prLoc );

	auto arLoc = _activeBeaconReceiveCands.find( sp );
	if (arLoc != _activeBeaconReceiveCands.end()) _activeBeaconReceiveCands.erase( arLoc );

	auto sLoc = _sendCands.find( sp );
	if (sLoc != _sendCands.end()) _sendCands.erase( sLoc );
}
