//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG_HANDSHAKE 1

#include <memory>
#include <chrono>
#include <list>
#include <iomanip>
#include <sstream>
#include <iterator>
#include "handshake.h"
#include "error_handling.h"

HandshakeChannel::HandshakeChannel( std::string name, GlobalVariables &globalVars ){

	_channelName = name;
	_globalVars = globalVars;
}

std::string HandshakeChannel::getChannelName(void){ return _channelName;}


std::shared_ptr<HandshakeCandidate> HandshakeChannel::buildHandshakeCandidate( std::shared_ptr<Candidate> sendCand, std::shared_ptr<Candidate> receiveCand, int sEval ){

	assert( (receiveCand -> actionCandidate) -> identify() == "MessageReceive");
	assert( (sendCand -> actionCandidate) -> identify() == "MessageSend");

#if DEBUG_HANDSHAKE
std::cout << "built handshake candidate on channel: " << _channelName << std::endl;
std::cout << "sending sp: " << sendCand -> processInSystem << std::endl;
std::cout << "receiving sp: " << receiveCand -> processInSystem << std::endl;
#endif

	std::map< std::string, double > augmentedLocalVars = receiveCand -> localVariables;

	//if we have a binding variable, we're allowed to use it in the rate calculation for the handshake receive candidate
	MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >(receiveCand -> actionCandidate);
	if ( mrb -> bindsVariable() ){

		augmentedLocalVars[ mrb -> getBindingVariable() ] = sEval;
	}

	double receiveRate = evalRPN_double( mrb -> getRate(), receiveCand -> parameterValues, _globalVars, augmentedLocalVars );
	if ( receiveRate <= 0 ) throw BadRate( mrb -> getToken() );

	double rate = (sendCand -> rate) * receiveRate;
	std::shared_ptr<HandshakeCandidate> hsCand( new HandshakeCandidate( sendCand, receiveCand, rate, sEval, _channelName ) );

	//associate both the sending and receiving system processes with this handshake candidate, and vice versa
	_possibleHandshakes_sp2Candidates[ sendCand -> processInSystem ].push_back( hsCand );
	_possibleHandshakes_sp2Candidates[ receiveCand -> processInSystem ].push_back( hsCand );
	_possibleHandshakes_candidates2Sp[ hsCand ].push_back( sendCand -> processInSystem );
	_possibleHandshakes_candidates2Sp[ hsCand ].push_back( receiveCand -> processInSystem );

	return hsCand;
}


std::pair<int, double> HandshakeChannel::updateHandshakeCandidates(void){

	int candidatesAdded = 0;
	double rateSum = 0.0;

	//match added send to receives that are already there
	for ( auto addedSend = _sendToAdd.begin(); addedSend != _sendToAdd.end(); addedSend++ ){

		assert( ((*addedSend) -> rangeEvaluation).size() == 1 );
		int sEval = *(((*addedSend) -> rangeEvaluation).begin());

		for ( auto receive = _hsReceive_Sp2Candidates.begin(); receive != _hsReceive_Sp2Candidates.end(); receive++ ){

			for ( auto r_cand = (receive -> second).begin(); r_cand != (receive -> second).end(); r_cand++ ){

				std::set<int> rEval = (*r_cand) -> rangeEvaluation;
				if ( rEval.count(sEval) > 0 ){

					std::shared_ptr<HandshakeCandidate> newHS = buildHandshakeCandidate( *addedSend, *r_cand, sEval );
					candidatesAdded++;
					rateSum += newHS -> rate;
				}
			}
		}
	}

	//match added receives to sends that are already there
	for ( auto addedReceive = _receiveToAdd.begin(); addedReceive != _receiveToAdd.end(); addedReceive++ ){

		std::set<int> rEval = (*addedReceive) -> rangeEvaluation;

		for ( auto send = _hsSend_Sp2Candidates.begin(); send != _hsSend_Sp2Candidates.end(); send++ ){

			for ( auto s_cand = (send -> second).begin(); s_cand != (send -> second).end(); s_cand++ ){

				assert( ( (*s_cand) -> rangeEvaluation).size() == 1 );
				int sEval = *(( (*s_cand) -> rangeEvaluation).begin());

				if ( rEval.count(sEval) > 0 ){

					std::shared_ptr<HandshakeCandidate> newHS = buildHandshakeCandidate( *s_cand, *addedReceive, sEval );
					candidatesAdded++;
					rateSum += newHS -> rate;
				}
			}
		}
	}

	//match added sends to added receives
	for ( auto addedSend = _sendToAdd.begin(); addedSend != _sendToAdd.end(); addedSend++ ){

		assert( ((*addedSend) -> rangeEvaluation).size() == 1 );
		int sEval = *(((*addedSend) -> rangeEvaluation).begin());

		for ( auto r_cand = _receiveToAdd.begin(); r_cand != _receiveToAdd.end(); r_cand++ ){

			std::set<int> rEval = (*r_cand) -> rangeEvaluation;
			if ( rEval.count(sEval) > 0 ){

				std::shared_ptr<HandshakeCandidate> newHS = buildHandshakeCandidate( *addedSend, *r_cand, sEval );
				candidatesAdded++;
				rateSum += newHS -> rate;
			}
		}
	}
	
	//add everything to sp -> candidates at the end so we don't count anything twice
	for ( auto addedSend = _sendToAdd.begin(); addedSend != _sendToAdd.end(); addedSend++ ){

		_hsSend_Sp2Candidates[(*addedSend) -> processInSystem].push_back( *addedSend );
	}
	for ( auto addedReceive = _receiveToAdd.begin(); addedReceive != _receiveToAdd.end(); addedReceive++ ){

		_hsReceive_Sp2Candidates[(*addedReceive) -> processInSystem].push_back( *addedReceive );		
	}

	_sendToAdd.clear();
	_receiveToAdd.clear();

	return std::make_pair(candidatesAdded, rateSum);
}


std::pair< int, double > HandshakeChannel::cleanSPFromChannel( SystemProcess *sp ){
//clean a system process that we're removing from the system from the channel
//return the number of handshake candidates we removed and the amount that this should decrease the total system rate

	int removed = 0;
	double rateSum = 0.0;

	//for each candidate the system process used
	if ( _possibleHandshakes_sp2Candidates.count(sp) > 0 ){
		for ( auto c = _possibleHandshakes_sp2Candidates.at(sp).begin(); c != _possibleHandshakes_sp2Candidates.at(sp).end(); c++ ){

			//count the candidate as removed and decrement the ratesum
			removed++;
			rateSum += (*c) -> rate;

			//remove this candidate from the sp->candidate map for other sp's that also use it so we don't count a candidate twice later
			assert(_possibleHandshakes_candidates2Sp.count(*c) > 0);

			std::list< SystemProcess * > otherSps = _possibleHandshakes_candidates2Sp[*c];
			for ( auto otherSp = otherSps.begin(); otherSp != otherSps.end(); otherSp++ ){

				if ( *otherSp == sp ) continue; //we'll do this one later
		
				//find the candidate in the candidate list for the other sp and erase it
				auto itr = std::find( _possibleHandshakes_sp2Candidates[*otherSp].begin(), _possibleHandshakes_sp2Candidates[*otherSp].end(), *c );
				_possibleHandshakes_sp2Candidates[*otherSp].erase( itr );
			}
			_possibleHandshakes_candidates2Sp.erase( _possibleHandshakes_candidates2Sp.find( *c ) );
		}
		_possibleHandshakes_sp2Candidates.erase( _possibleHandshakes_sp2Candidates.find(sp) );
	}

	auto locInSend = _hsSend_Sp2Candidates.find( sp );
	if (locInSend != _hsSend_Sp2Candidates.end() ){

#if DEBUG_HANDSHAKE
std::cout << "chan " << _channelName << " removed " << _hsSend_Sp2Candidates[sp].size() << " possible sends associated with " << sp << std::endl;
#endif
		_hsSend_Sp2Candidates.erase( locInSend );
	}

	auto locInRec = _hsReceive_Sp2Candidates.find( sp );
	if (locInRec != _hsReceive_Sp2Candidates.end() ){

#if DEBUG_HANDSHAKE
std::cout << "chan " << _channelName << " removed " << _hsReceive_Sp2Candidates[sp].size() << " possible receives associated with " << sp << std::endl;
#endif
		_hsReceive_Sp2Candidates.erase( locInRec );
	}

#if DEBUG_HANDSHAKE
std::cout << "cleaned " << sp << " and removed " << removed << std::endl;
#endif
	return std::make_pair( removed, rateSum );
}


std::shared_ptr<HandshakeCandidate> HandshakeChannel::pickCandidate(double &runningTotal, double uniformDraw, double rateSum){

	for ( auto cand = _possibleHandshakes_candidates2Sp.begin(); cand != _possibleHandshakes_candidates2Sp.end(); cand++ ){

		double r = (cand -> first) -> rate;
		double lower = runningTotal / rateSum;
		double upper = (runningTotal + r) / rateSum;

		if ( uniformDraw > lower and uniformDraw <= upper ){

			return cand -> first;
		}
		runningTotal += r;
	}
	return NULL;
}


void HandshakeChannel::addSendCandidate( std::shared_ptr<Candidate> sc ){

#if DEBUG_HANDSHAKE
std::cout << "adding hs send candidate on channel: " << _channelName << std::endl;
std::cout << "associated with sp: " << sc -> processInSystem << std::endl;
#endif
	_sendToAdd.push_back( sc );
}


void HandshakeChannel::addReceiveCandidate( std::shared_ptr<Candidate> rc ){

#if DEBUG_HANDSHAKE
std::cout << "adding hs receive candidate on channel: " << _channelName << std::endl;
std::cout << "associated with sp: " << rc -> processInSystem << std::endl;
#endif
	_receiveToAdd.push_back( rc );
}
