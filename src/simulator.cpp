//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG 1

#include <functional>
#include <cassert>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include "blockParser.h"
#include "error_handling.h"
#include "simulator.h"
#include "evaluate_trees.h"
#include "common.h"

class NegativeLog : public std::exception {
	public:
		virtual const char * what () const throw () {
			return "Negative value passed to natural log function.";
		}
};

//From DNAscent/src/probability.cpp
bool lnGreaterThan( double ln_x, double ln_y ){
/*evalutes whether ln_x is greater than ln_y, and returns a boolean */

	if ( std::isnan( ln_x ) || std::isnan( ln_y ) ){

		if ( std::isnan( ln_x ) || std::isnan( ln_y ) == false ){
			return false;
		}
        	else if ( std::isnan( ln_x ) == false || std::isnan( ln_y ) ){
			return true;
		}
		else{
			return false;
		}
	}
	else{
		if ( ln_x > ln_y ){
			return true;
		}
		else{
			return false;
		}
	}
}

bool lnGreaterThanOrEq( double ln_x, double ln_y ){
/*evalutes whether ln_x is greater than ln_y, and returns a boolean */

	if ( std::isnan( ln_x ) || std::isnan( ln_y ) ){

		if ( std::isnan( ln_x ) || std::isnan( ln_y ) == false ){
			return false;
		}
        	else if ( std::isnan( ln_x ) == false || std::isnan( ln_y ) ){
			return true;
		}
		else{
			return false;
		}
	}
	else{
		if ( ln_x >= ln_y ){
			return true;
		}
		else{
			return false;
		}
	}
}

//From DNAscent/src/probability.cpp
double eexp( double x ){
/*Map from log space back to linear space. */

	if ( std::isnan( x ) ) {
		return 0.0;
	}
	else {
		return exp( x );
	}
}

//From DNAscent/src/probability.cpp
double eln( double x ){
/*Map from linear space to log space. */

	if (x == 0.0){
		return NAN;
	}
	else if (x > 0.0){
		return log( x );
	}
	else{
		throw NegativeLog();
	}
}

double logSumExp(double a, double b){
// adds a and b in log space
// note that inputs are logs of original summands

	//set u as the larger of the two arguments
	double u,v;
	if (lnGreaterThan(a,b)){
	
		u = a;
		v = b; 
	}
	else{
	
		u = b;
		v = a; 	
	}
	
	double sum = u + eln(1 + eexp(v-u));
	return sum;
}

double logSumExp_decrement(double u, double v){
// decrements u by v
// equivalent to u -= v
// u must be geq than v

	assert(lnGreaterThanOrEq(u,v));
	assert(lnGreaterThanOrEq(u,NAN));
	assert(lnGreaterThanOrEq(v,NAN));

	double sum = u + eln(1 - eexp(v-u));
	return sum;
}

System::System( std::list< SystemProcess > &s, std::map< std::string, ProcessDefinition > &processDefs, int mT, double mD, GlobalVariables &globalVars ){

	_name2ProcessDef = processDefs;
	_maxTransitions = mT;
	_maxDuration = mD;
	_globalVars = globalVars;

	for ( auto i = s.begin(); i != s.end(); i++ ){

		_currentProcesses.push_back( new SystemProcess( *i ) );
	}

	//do an initial pass through the whole system
	std::list< SystemProcess * > newProcesses;
	for ( auto sp = _currentProcesses.begin(); sp != _currentProcesses.end(); sp++ ){

		//see if we can make multiple system processes out of this one by splitting on parallel operators
		if ( ((*sp) -> parseTree).getRoot() -> identify() == "Parallel" ){

			splitOnParallel( *sp, ((*sp) -> parseTree).getRoot(), newProcesses );
			delete *sp;
			sp = _currentProcesses.erase( sp );
		}
	}
	_currentProcesses.insert( _currentProcesses.end(), newProcesses.begin(), newProcesses.end() );

	//sum the transition rates for non-handshake candidates while building a list of handshake candidates
	std::list< SystemProcess > parallelProcesses;
	for ( auto s = _currentProcesses.begin(); s != _currentProcesses.end(); s++ ){

		sumTransitionRates( *s, (*s) -> parseTree, ((*s) -> parseTree).getRoot(), parallelProcesses, (*s) -> parameterValues );
	}

	//sum handshake transitions
	for ( auto chan = _handshakes_Name2Channel.begin(); chan != _handshakes_Name2Channel.end(); chan++ ){

		int newHandshakesAdded;
		double rateSumIncrease;
		std::tie(newHandshakesAdded,rateSumIncrease) = (chan -> second) -> updateHandshakeCandidates();
		_candidatesLeft += newHandshakesAdded;
		_rateSum += rateSumIncrease;
	}
}


std::string System::writeChannelName( std::vector< std::vector< Token * > > channelName ){

	std::string out;
	for ( auto i = channelName.begin(); i < channelName.end(); i++ ){ //for each comma-separated value

		for ( auto j = (*i).begin(); j < (*i).end(); j++ ){ //for each token in that value

			out += (*j) -> value();
		}
		if (i != channelName.end() - 1) out += ',';
	}
	return out;
}


void System::writeTransition( double time, std::shared_ptr<Candidate> chosen, std::stringstream &ss ){

	Block *actionDone = chosen -> actionCandidate;
	std::vector< std::string > parameterNames = _name2ProcessDef[ actionDone -> getOwningProcess()].parameters;

	if ( actionDone -> identify() == "Action" ){

		ActionBlock *ab = static_cast< ActionBlock * >( actionDone );
		ss << time << '\t' << ab -> actionName << '\t' << actionDone -> getOwningProcess();
	}
	else if ( actionDone -> identify() == "MessageSend" ){

		MessageSendBlock *msb = static_cast< MessageSendBlock * >( actionDone );
		ss << time << '\t' << writeChannelName(msb -> getChannelName()) << '\t' << actionDone -> getOwningProcess();
	}
	else if ( actionDone -> identify() == "MessageReceive" ){

		MessageReceiveBlock *mrb = static_cast< MessageReceiveBlock * >( actionDone );
		ss << time << '\t' << writeChannelName(mrb -> getChannelName())  << '\t' << actionDone -> getOwningProcess();
	}

	for ( auto p = parameterNames.begin(); p < parameterNames.end(); p++ ){

		if ( (chosen -> parameterValues).values.count(*p) > 0 ){
	
			Numerical val = (chosen -> parameterValues).values[*p];

			if (val.isInt()) ss << '\t' << *p << '\t' << val.getInt();
			else ss << '\t' << *p << '\t' << val.getDouble();
		}
	}
	ss << std::endl;
}


//for debugging
void System::printTransition(double time, std::shared_ptr<Candidate> chosen){

	Block *actionDone = chosen -> actionCandidate;
	std::vector< std::string > parameterNames = _name2ProcessDef[ actionDone -> getOwningProcess()].parameters;

	if ( actionDone -> identify() == "Action" ){

		ActionBlock *ab = static_cast< ActionBlock * >( actionDone );
		std::cout << time << '\t' << ab -> actionName << '\t' << actionDone -> getOwningProcess();
	}
	else if ( actionDone -> identify() == "MessageSend" ){

		MessageSendBlock *msb = static_cast< MessageSendBlock * >( actionDone );
		std::cout << time << '\t' << writeChannelName(msb -> getChannelName()) << '\t' << actionDone -> getOwningProcess();
	}
	else if ( actionDone -> identify() == "MessageReceive" ){

		MessageReceiveBlock *mrb = static_cast< MessageReceiveBlock * >( actionDone );
		std::cout << time << '\t' << writeChannelName(mrb -> getChannelName())  << '\t' << actionDone -> getOwningProcess();
	}

	for ( auto p = parameterNames.begin(); p < parameterNames.end(); p++ ){

		if ( (chosen -> parameterValues).values.count(*p) > 0 ){
	
			Numerical val = (chosen -> parameterValues).values[*p];

			if (val.isInt()) std::cout << '\t' << *p << '\t' << val.getInt();
			else std::cout << '\t' << *p << '\t' << val.getDouble();
		}
	}
	std::cout << std::endl;
}


bool System::variableIsDefined(std::string varName, ParameterValues &currentParameters, std::map< std::string, Numerical > &localVariables){

	bool inGlobal =  _globalVars.values.count(varName) > 0;
	bool inParams =  currentParameters.values.count(varName) > 0;
	bool inLocal = localVariables.count(varName) > 0;
	if (not inGlobal and not inParams and not inLocal) return false;
	else return true;
}


std::vector< std::string > System::substituteChannelName( std::vector< std::vector< Token * > > channelExpressions, ParameterValues &currentParameters, std::map< std::string, Numerical > &localVariables ){

	std::vector< std::string > channelName;

	for (auto exp = channelExpressions.begin(); exp < channelExpressions.end(); exp++ ){

		if ( (*exp).size() == 1 and (*exp)[0] -> identify() == "Variable" and not variableIsDefined( (*exp)[0] -> value(), currentParameters, localVariables) ){

			channelName.push_back( (*exp)[0] -> value() );
		}

		else{ //this is an expression or variable that should be substituted
		
			Numerical evalIdx = evalRPN_numerical(*exp, currentParameters, _globalVars, localVariables);
			if (evalIdx.isDouble()) throw WrongType((*exp)[0], "Channel name expressions must evaluate to ints, not doubles (either through explicit or implicit casting).");
			channelName.push_back( std::to_string(evalIdx.getInt()) );
		}
	}
	return channelName;
}


void System::sumTransitionRates( SystemProcess *sp,
			                     Tree<Block> &bt,
			                     Block *current,
			                     std::list< SystemProcess > parallelProcesses,
			                     ParameterValues &currentParameters){

	if ( current -> identify() == "Action" ){

		Numerical rate = evalRPN_numerical( current -> getRate(), currentParameters, _globalVars, sp -> localVariables );
		if ( rate.doubleCast() <= 0 ) throw BadRate( current -> getToken() );
		std::shared_ptr<Candidate> cand( new Candidate( current, currentParameters, sp -> localVariables, sp, parallelProcesses ) );
		cand -> rate = rate.doubleCast();
		_nonMsgCandidates[sp].push_back( cand );
		_candidatesLeft += sp -> clones;
		_rateSum += rate.doubleCast() * (sp -> clones);
	}
	else if ( current -> identify() == "MessageSend" ){

		MessageSendBlock *msb = static_cast< MessageSendBlock * >( current );
		std::vector< std::string > channelName = substituteChannelName( msb -> getChannelName(), currentParameters, sp -> localVariables );

		if ( msb -> isHandshake() ){

			Numerical rate = evalRPN_numerical( msb -> getRate(), currentParameters, _globalVars, sp -> localVariables );
			if ( rate.doubleCast() <= 0 ) throw BadRate( current -> getToken() );

			std::shared_ptr< Candidate > cand( new Candidate( msb, currentParameters, sp -> localVariables, sp, parallelProcesses) );

			cand -> rate = rate.doubleCast();

			//evaluate each parameter expression
			std::vector< std::vector< Token * > > parameterExpressions = msb -> getParameterExpression();
			for ( auto exp = parameterExpressions.begin(); exp < parameterExpressions.end(); exp++ ){

				Numerical paramEval = evalRPN_numerical( *exp, currentParameters, _globalVars, sp -> localVariables );
				(cand -> sendReceiveParameters).push_back(paramEval.getInt());
			}

			if ( _handshakes_Name2Channel.find( channelName ) != _handshakes_Name2Channel.end() ){

				_handshakes_Name2Channel[channelName] -> addSendCandidate(cand);
			}
			else{
				std::shared_ptr< HandshakeChannel > newChannel(new HandshakeChannel(channelName, _globalVars));
				_handshakes_Name2Channel[channelName] = newChannel;
				_handshakes_Name2Channel[channelName] -> addSendCandidate(cand);
			}
		}
		else{//beacon launch or kill

			if ( _beacons_Name2Channel.find( channelName ) != _beacons_Name2Channel.end() ){

				_beacons_Name2Channel[channelName] -> addCandidate( current, sp, parallelProcesses, currentParameters, _candidatesLeft, _rateSum );
			}
			else{
				std::shared_ptr< BeaconChannel > newChannel( new BeaconChannel(channelName, _globalVars) );
				_beacons_Name2Channel[channelName] = newChannel;
				_beacons_Name2Channel[channelName] -> addCandidate( current, sp, parallelProcesses, currentParameters, _candidatesLeft, _rateSum );
			}
		}
	}
	else if ( current -> identify() == "MessageReceive" ){

		MessageReceiveBlock *mrb = static_cast< MessageReceiveBlock * >( current );
		std::vector< std::string > channelName = substituteChannelName( mrb -> getChannelName(), currentParameters, sp -> localVariables );

		if ( mrb -> isHandshake() ){

			std::shared_ptr< Candidate > cand( new Candidate( mrb, currentParameters, sp -> localVariables, sp, parallelProcesses) );

			if ( _handshakes_Name2Channel.find( channelName ) != _handshakes_Name2Channel.end() ){

				_handshakes_Name2Channel[channelName] -> addReceiveCandidate(cand);
			}
			else{
				std::shared_ptr< HandshakeChannel > newChannel( new HandshakeChannel(channelName, _globalVars) );
				_handshakes_Name2Channel[channelName] = newChannel;
				_handshakes_Name2Channel[channelName] -> addReceiveCandidate(cand);
			}
		}
		else{//beacon receive or beacon check

			if ( _beacons_Name2Channel.find( channelName ) != _beacons_Name2Channel.end() ){

				_beacons_Name2Channel[channelName] -> addCandidate( current, sp, parallelProcesses, currentParameters, _candidatesLeft, _rateSum );
			}
			else{

				std::shared_ptr< BeaconChannel > newChannel( new BeaconChannel(channelName, _globalVars) );
				_beacons_Name2Channel[channelName] = newChannel;
				_beacons_Name2Channel[channelName] -> addCandidate( current, sp, parallelProcesses, currentParameters, _candidatesLeft, _rateSum );
			}
		}
	}
	else if ( current -> identify() == "Gate" ){

		GateBlock *gb = static_cast< GateBlock * >( current );
		bool gateConditionHolds = evalRPN_condition( gb -> getConditionExpression(), currentParameters, _globalVars, sp -> localVariables );
		if ( gateConditionHolds ){

			std::vector< Block * > children = bt.getChildren(current);
			assert( children.size() == 1 );//gates are unary
			sumTransitionRates( sp, bt, children[0], parallelProcesses, currentParameters );
		}
	}
	else if ( current -> identify() == "Process" ){

		ProcessBlock *pb = static_cast< ProcessBlock * >(current);

		//update the parameter values based on any process arithmetic we're doing
		ParameterValues oldParameterValues = currentParameters;
		std::vector< std::string > parameterNames = _name2ProcessDef[ pb -> getProcessName()].parameters;
		for ( unsigned int i = 0; i < parameterNames.size(); i++ ){

			currentParameters.updateValue( parameterNames[i], evalRPN_numerical(pb -> getParameterExpressions()[i], oldParameterValues , _globalVars, sp -> localVariables) );
		}
		//recurse down using this process's tree and the updated parameter values
		Tree<Block> newTree = _name2ProcessDef[ pb -> getProcessName()].parseTree;
		sumTransitionRates( sp, newTree, newTree.getRoot(), parallelProcesses, currentParameters );
	}
	else if ( current -> identify() == "Parallel" ){

		std::vector< Block * > children = bt.getChildren( current );

		//left child
		std::list< SystemProcess > forLeft = parallelProcesses;
		SystemProcess left_sp = SystemProcess( *sp );
		left_sp.parseTree = bt.getSubtree( children[1] );
		//printBlockTree(left_sp.parseTree,left_sp.parseTree.getRoot());
		//std::cout << children[1] -> identify() << std::endl;
		forLeft.push_back( left_sp );
		sumTransitionRates( sp, bt, children[0], forLeft, currentParameters );

		//right child
		SystemProcess right_sp = SystemProcess( *sp );
		right_sp.parseTree = bt.getSubtree( children[0] );
		//printBlockTree(right_sp.parseTree,right_sp.parseTree.getRoot());
		//std::cout << children[0] -> identify() << std::endl;
		parallelProcesses.push_back( right_sp );
		sumTransitionRates( sp, bt, children[1], parallelProcesses, currentParameters );
		//NOTE: the indexing for children looks weird, but it's fine and it's also checked by the process-parallelTreeRecursion.bc test
	}
	else {
		std::vector< Block * > children = bt.getChildren(current);
		for ( auto c = children.begin(); c < children.end(); c++ ){

			sumTransitionRates( sp, bt, *c, parallelProcesses, currentParameters );
		}
	}
}


void System::getParallelProcesses( std::shared_ptr<Candidate> chosen, std::list< SystemProcess * > &toAdd ){
//if we choose this candidate, get the processes that would act in parallel to this one

	//add parallel processes to the system
	for ( auto pp = (chosen -> parallelProcesses).begin(); pp != (chosen -> parallelProcesses).end(); pp++ ){

		toAdd.push_back( new SystemProcess( *pp ) );
	}
}


SystemProcess * System::updateSpForTransition( std::shared_ptr<Candidate> chosen ){

	SystemProcess *SPtoModify = chosen -> processInSystem;

	//get the child of the chosen action, and update the current system process so that it starts from there
	Block *actionDone = chosen -> actionCandidate;
	Tree<Block> treeForAction = _name2ProcessDef[ actionDone -> getOwningProcess() ].parseTree;

	if ( treeForAction.isLeaf( actionDone ) ){
		return NULL;
	}
	else {

		SystemProcess *newSp = new SystemProcess( *SPtoModify );
		newSp -> clones = 1;
		newSp -> parameterValues = chosen -> parameterValues; //inherit the parameter variables from the candidate
		std::vector< Block * > children = treeForAction.getChildren( actionDone );
		assert( children.size() == 1 );
		newSp -> parseTree = treeForAction.getSubtree( children[0] );
#if DEBUG
std::cout << "   Update for transition: killed " << SPtoModify << " and added " << newSp << std::endl;
#endif
		return newSp;
	}
}


void System::splitOnParallel(SystemProcess *sp, Block *currentNode, std::list< SystemProcess * > &toAdd ){
//recurse down a parse tree, get the first blocks that aren't parallel operators, and make separate system processes for them
//prevents issues in situations where we have handshakes between two message actions within a single system process

	if ( currentNode -> identify() == "Parallel" ){

		std::vector< Block * > children = (sp -> parseTree).getChildren( currentNode );
		for ( auto c = children.begin(); c < children.end(); c++ ){

			splitOnParallel(sp, *c, toAdd );
		}
	}
	else {

		SystemProcess *newSp = new SystemProcess( *sp );
		newSp -> parseTree = (sp -> parseTree).getSubtree( currentNode );
		toAdd.push_back( newSp );

#if DEBUG
std::cout << "   Splitting on parallel, removed " << sp << " and added " << newSp << std::endl;
#endif

		return;
	}
}


void System::removeChosenFromSystem( std::shared_ptr<Candidate> candToRemove, bool databaseUpdated ){

	SystemProcess *sp = candToRemove -> processInSystem;

	//take away all the rates that this system contributed to the rateSum
	bool inNonMsgCandidates = false;
	for ( auto c = _nonMsgCandidates[sp].begin(); c != _nonMsgCandidates[sp].end(); c++ ){

		_rateSum -= (*c) -> rate;
		_candidatesLeft--;
		inNonMsgCandidates = true;
	}

	//erase from candidates
	if ( inNonMsgCandidates and sp -> clones == 1 ) _nonMsgCandidates.erase( sp );

	//erase any beacon candidate that pertains to sp
	for ( auto be = _beacons_Name2Channel.begin(); be != _beacons_Name2Channel.end(); be++ ){

		(be -> second) -> cleanSPFromChannel(sp,_candidatesLeft,_rateSum);
	}

	//erase any handshake candidate that could be sent or received from sp
	for ( auto hs = _handshakes_Name2Channel.begin(); hs != _handshakes_Name2Channel.end(); hs++ ){

		int handshakesRemoved;
		double rateSumDecrease; 
		std::tie(handshakesRemoved,rateSumDecrease) = (hs -> second) -> cleanSPFromChannel(sp);
		_rateSum -= rateSumDecrease;
		_candidatesLeft -= handshakesRemoved;
	}

	//reshuffle potential vs active beacon receives, but only do this if database was updated, and only do it on the channel that was updated
	if (databaseUpdated){

		_beacons_Name2Channel[candToRemove -> beaconChannelName] -> updateBeaconCandidates(_candidatesLeft,_rateSum);
	}

#if DEBUG
std::cout << "   Removing chosen from system " << sp << std::endl;
std::cout << "   It has clones: " << sp -> clones << std::endl;
#endif

	if (sp -> clones == 1){

#if DEBUG
std::cout << "   Deleting system process " << sp << std::endl;
std::cout << "   It has clones: " << sp -> clones << std::endl;
#endif

		//remove the system process from the system
		_currentProcesses.erase( std::find(_currentProcesses.begin(), _currentProcesses.end(), sp ) );
		delete sp;
	}
	else{

		sp -> clones -= 1;
#if DEBUG
std::cout << "   Reducing system process clones for " << sp << std::endl;
std::cout << "   It now has clones: " << sp -> clones << std::endl;
#endif
	}
}


bool System::condenseSystem(SystemProcess *sp){

	//get all the system processes that match on parse trees, parameter values, and local variables
	std::vector<SystemProcess *> matchingProcesses;

	std::list<SystemProcess *>::iterator pos = std::find_if(_currentProcesses.begin(),_currentProcesses.end(),findSp(sp));
	while (pos != _currentProcesses.end()){

		if (*pos != sp) matchingProcesses.push_back(*pos);
		pos++;
		pos = std::find_if(pos,_currentProcesses.end(),findSp(sp));
	}

	//Non-messaging actions - check if candidates all have the same parallel processes by matching them up on the block pointers
	std::vector< std::shared_ptr<Candidate> > sp_candidates = _nonMsgCandidates[sp];
	std::vector<SystemProcess *> matchOnNonMsg;
	for (auto mp = matchingProcesses.begin(); mp < matchingProcesses.end();){

		std::vector< std::shared_ptr<Candidate> > mp_candidates = _nonMsgCandidates[*mp];

		//because the sp's are the same, the parameter values, local variables already match, and block tree already match
		//just check the parallel process so that they have the same history
		//e.g., for process F, it might have different parallel processes if P starts F versus if F starts by itself
		bool matchNonMsg = std::is_permutation(sp_candidates.begin(), sp_candidates.end(), mp_candidates.begin(), compareCandidates);
		if (not matchNonMsg) mp = matchingProcesses.erase(mp);
		else mp++;
	}

	//handshake actions - check if candidates all have the same parallel processes by matching them up on the block pointers
	if (matchingProcesses.size() > 0){
		for (auto mp = matchingProcesses.begin(); mp < matchingProcesses.end();){

			bool matchesOnHandshakes = true;
			for ( auto hs = _handshakes_Name2Channel.begin(); hs != _handshakes_Name2Channel.end(); hs++ ){

				bool matchesThisChannel = (hs -> second) -> matchClone( sp, *mp);
				if (not matchesThisChannel) matchesOnHandshakes = false;
				break;
			}
			if (not matchesOnHandshakes) mp = matchingProcesses.erase(mp);
			else mp++;
		}
	}

	//beacon actions - check if candidates all have the same parallel processes by matching them up on the block pointers
	if (matchingProcesses.size() > 0){
		for (auto mp = matchingProcesses.begin(); mp < matchingProcesses.end();){
			bool matchesOnBeacons = true;
			for ( auto be = _beacons_Name2Channel.begin(); be != _beacons_Name2Channel.end(); be++ ){

				bool matchesThisChannel = (be -> second) -> matchClone( sp, *mp);
				if (not matchesThisChannel) matchesOnBeacons = false;
				break;
			}
			if (not matchesOnBeacons) mp = matchingProcesses.erase(mp);
			else mp++;
		}
	}

	assert(matchingProcesses.size() == 0 || matchingProcesses.size() == 1);

	//found a match so clean up
	if (matchingProcesses.size() == 1){

		SystemProcess *mp = matchingProcesses[0];
		mp -> clones += 1;

		//delete from non messaging actions
		if (_nonMsgCandidates.count(sp) > 0) _nonMsgCandidates.erase( sp );

		//delete from handshakes
		for ( auto hs = _handshakes_Name2Channel.begin(); hs != _handshakes_Name2Channel.end(); hs++ ){

			(hs -> second) -> cleanCloneFromChannel(sp);
		}

		//delete from handshakes
		for ( auto be = _beacons_Name2Channel.begin(); be != _beacons_Name2Channel.end(); be++ ){

			(be -> second) -> cleanCloneFromChannel(sp);
		}

#if DEBUG
std::cout << "   Condensed system" << std::endl;
std::cout << "   Match for " << sp << std::endl;
std::cout << "   found at existing " << mp << std::endl;
std::cout << "   Current clones of match " << mp -> clones << std::endl;
#endif

		return true;
	}
	else return false;
}


void System::simulate(void){

	while ( _candidatesLeft > 0 and _transitionsTaken < _maxTransitions and _totalTime <= _maxDuration ){

		/*draw time of next transition */
		std::random_device rd;
		std::mt19937 rnd_gen( rd() );
		std::exponential_distribution< double > expDist(_rateSum);
		double exponentialDraw = expDist(rnd_gen);
		_totalTime += exponentialDraw;

#if DEBUG
std::cout << "=====================================================" << std::endl;
std::cout << "Candidates left: " << _candidatesLeft << std::endl;
std::cout << "Transitions taken: " << _transitionsTaken << std::endl;
std::cout << "Rate sum: " << _rateSum << std::endl;
std::cout << "Total time elapsed: " << _totalTime << std::endl;
#endif

		/*Monte Carlo step to decide next transition */
		std::uniform_real_distribution< double > uniDist(0.0, 1.0);
		double uniformDraw = uniDist(rnd_gen);

		/*go through all the transition candidates and stop when we find the correct one */
		bool firstRate = true;
		double runningTotal = 0.0;
		double log_runningTotal;
		bool found = false;
		std::list< SystemProcess * > toAdd;

		/*non-messaging choice */
		for ( auto s = _nonMsgCandidates.begin(); s != _nonMsgCandidates.end(); s++ ){

			std::vector< std::shared_ptr<Candidate> > candidates = s -> second;
			SystemProcess *sp = s -> first;
			size_t multiplier = sp -> clones;

			for ( auto tc = candidates.begin(); tc < candidates.end(); tc++ ){

				double lower = runningTotal / _rateSum;
				double uppersum = logSumExp(eln(runningTotal)+ eln(multiplier * ( (*tc) -> rate)))
				double upper = uppersum / _rateSum;

				if ( uniformDraw > lower and uniformDraw <= upper ){
#if DEBUG
std::cout << ">Candidate picked: non-msg action ";
Block *b = (*tc) -> actionCandidate;
Token *t = b -> getToken();
std::cout << t -> value();
std::cout << " at rate " << (*tc) -> rate << std::endl;
#endif
					//designate the chosen one
					getParallelProcesses( *tc, toAdd );
					SystemProcess *newSp = updateSpForTransition( *tc );
					if ( newSp ) toAdd.push_back(newSp);
					writeTransition( _totalTime, *tc, _outputStream );
#if DEBUG
printTransition(_totalTime, *tc);
#endif
					removeChosenFromSystem(*tc, false);
					found = true;
					goto foundCand;
				}
				else runningTotal = logSumExp(eln((*tc) -> rate * multiplier), eln(runningTotal));
			}
		}

		/*if we haven't chosen from the non-messaging choices, look at beacon action */
		for ( auto chanPair = _beacons_Name2Channel.begin(); chanPair != _beacons_Name2Channel.end(); chanPair++ ){ 
			
			std::shared_ptr<Candidate> beaconCand = (chanPair -> second) -> pickCandidate(runningTotal, uniformDraw, _rateSum);
			if ( beaconCand != NULL ){

#if DEBUG
std::cout << ">Candidate picked: beacon ";
Block *b = beaconCand -> actionCandidate;
Token *t = b -> getToken();
std::cout << t -> value();
std::cout << " at rate " << beaconCand -> rate << std::endl;
#endif

				getParallelProcesses( beaconCand, toAdd );
				SystemProcess *newSp = updateSpForTransition( beaconCand );

				if ( newSp and (beaconCand -> actionCandidate) -> identify() == "MessageReceive" ){

					//bind a new variable if applicable
					MessageReceiveBlock *mrb = static_cast< MessageReceiveBlock * >( beaconCand -> actionCandidate );
					if ( mrb -> bindsVariable() ){

						std::vector< std::string > bindingVars = mrb -> getBindingVariable();
						for ( unsigned int i = 0; i < bindingVars.size(); i++ ){
						
							Numerical n;
							n.setInt((beaconCand -> sendReceiveParameters)[i]);
							newSp -> localVariables[ bindingVars[i] ] = n;
						}
					}
				}

				if ( newSp ) toAdd.push_back(newSp);
				writeTransition( _totalTime, beaconCand, _outputStream );
#if DEBUG
printTransition(_totalTime, beaconCand);
#endif

				bool databaseUpdated = (beaconCand -> actionCandidate) -> identify() == "MessageSend";
				removeChosenFromSystem(beaconCand, databaseUpdated);
				found = true;
				goto foundCand;
			}
		}

		/*if we haven't chosen from the beacon actions, look at the handshakes */
		for ( auto chanPair = _handshakes_Name2Channel.begin(); chanPair != _handshakes_Name2Channel.end(); chanPair++ ){ 
			
			std::shared_ptr<HandshakeCandidate> hsCand = (chanPair -> second) -> pickCandidate(runningTotal, uniformDraw, _rateSum);
			
			if ( hsCand != NULL ){

#if DEBUG
std::cout << ">Candidate picked: handshake ";
Block *b = (hsCand -> hsSendCand) -> actionCandidate;
Token *t = b -> getToken();
std::cout << t -> value() << " ";
b = (hsCand -> hsReceiveCand) -> actionCandidate;
t = b -> getToken();
std::cout << t -> value();
std::cout << " at rate " << hsCand -> rate << std::endl;
#endif

				//handshake send
				getParallelProcesses( hsCand -> hsSendCand, toAdd );
				SystemProcess *newSp_send = updateSpForTransition( hsCand -> hsSendCand );
				if ( newSp_send ) toAdd.push_back(newSp_send);

				//handshake receive
				getParallelProcesses( hsCand -> hsReceiveCand, toAdd );
				SystemProcess *newSp_receive = updateSpForTransition( hsCand -> hsReceiveCand );

				if ( newSp_receive ){

					//bind a new variable if applicable
					MessageReceiveBlock *mrb = static_cast< MessageReceiveBlock * >( (hsCand -> hsReceiveCand) -> actionCandidate );
					if ( mrb -> bindsVariable() ){

						std::vector< std::string > bindingVars = mrb -> getBindingVariable();
						std::vector< int > receivedParams = hsCand -> getReceivedParam();
						for ( unsigned int i = 0; i < bindingVars.size(); i++ ){

							Numerical  n;
							n.setInt(receivedParams[i]);
							newSp_receive -> localVariables[ bindingVars[i] ] = n;
						}
					}
					toAdd.push_back(newSp_receive);
				}

				writeTransition( _totalTime, hsCand -> hsSendCand, _outputStream );
				writeTransition( _totalTime, hsCand -> hsReceiveCand, _outputStream );
#if DEBUG
printTransition(_totalTime, hsCand -> hsSendCand);
printTransition(_totalTime, hsCand -> hsReceiveCand);
#endif
				//remove handshake from the system
				removeChosenFromSystem( hsCand -> hsSendCand, false );
				removeChosenFromSystem( hsCand -> hsReceiveCand, false );

				found = true;
				goto foundCand;
			}
		}

		foundCand:
		assert( found );
		_transitionsTaken++;

#if DEBUG
std::cout << "   Reformatting system... " << std::endl;
#endif

		//re-format the system
		std::list< SystemProcess * > newProcesses;
		for ( auto s = toAdd.begin(); s != toAdd.end(); ){

			//see if we can make multiple system processes out of this one by splitting on parallel operators
			if ( ( (*s) -> parseTree).getRoot() -> identify() == "Parallel" ){				

				splitOnParallel( *s, ((*s) -> parseTree).getRoot(), newProcesses );
				delete *s;
				s = toAdd.erase( s );
			}
			else s++;
		}
		toAdd.insert( toAdd.end(), newProcesses.begin(), newProcesses.end() );

#if DEBUG
std::cout << "Done." << std::endl;
std::cout << "   Re-summing transitions... " << std::endl;
#endif

		//sum the transition rates for non-handshake candidates while building a list of handshake candidates
		std::list< SystemProcess > parallelProcesses;
		for ( auto s = toAdd.begin(); s != toAdd.end(); s++ ){

			sumTransitionRates( *s, (*s) -> parseTree, ((*s) -> parseTree).getRoot(), parallelProcesses, (*s) -> parameterValues );
		}

#if DEBUG
std::cout << "Done." << std::endl;
std::cout << "   Re-summing handshakes... " << std::endl;
#endif

		//sum handshake transitions
		for ( auto chan = _handshakes_Name2Channel.begin(); chan != _handshakes_Name2Channel.end(); chan++ ){

			int newHandshakesAdded;
			double rateSumIncrease;
			std::tie(newHandshakesAdded,rateSumIncrease) = (chan -> second) -> updateHandshakeCandidates();
			_candidatesLeft += newHandshakesAdded;
			_rateSum += rateSumIncrease;
		}

#if DEBUG
std::cout << "Done." << std::endl;
std::cout << "   Condensing system processes... " << std::endl;
#endif

		for ( auto s = toAdd.begin(); s != toAdd.end(); ){

			bool condensed = condenseSystem(*s);
			if (condensed){
				delete *s;
				s = toAdd.erase(s);
			}
			else s++;
		}

#if DEBUG
std::cout << "Done." << std::endl;
std::cout << "Total processes added: " << toAdd.size() << std::endl;
for (auto a = toAdd.begin(); a != toAdd.end(); a++) std::cout << *a << std::endl;
#endif

		_currentProcesses.insert( _currentProcesses.end(), toAdd.begin(), toAdd.end() );
	}
}


void simulateSystem( std::map< std::string, ProcessDefinition > &name2ProcessDef, std::list< SystemProcess > &system, GlobalVariables &globalVars, int numOfSimulations, int threads, std::string outputFilename, int maxTransitions, double maxDuration ){

	std::ofstream outFile( outputFilename );
	progressBar pb( numOfSimulations );
	int numCompleted = 0;

	/*each simulation */
	#pragma omp parallel for schedule(dynamic) shared(pb, system, globalVars, numCompleted) num_threads( threads )
	for ( int i = 0; i < numOfSimulations; i++ ){

		System systemLocal( system, name2ProcessDef, maxTransitions, maxDuration, globalVars );
		systemLocal.simulate();
		numCompleted++;

		#pragma omp critical 
		{
		pb.displayProgress( numCompleted );
		outFile << ">=======" << std::endl;
		outFile << systemLocal.write();
		}
	}
	std::cout << std::endl;
}
