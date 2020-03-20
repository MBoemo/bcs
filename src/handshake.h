//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include <memory>
#include <map>
#include <chrono>
#include <list>
#include <iomanip>
#include <sstream>
#include <iterator>
#include "evaluate_trees.h"

class HandshakeCandidate{

	public:
		std::shared_ptr<Candidate> hsSendCand, hsReceiveCand;
		std::vector< int > receivedParam;
		bool bindsVariable = false;
		std::string bindingVariable;
		double rate;
		std::vector< std::string > channel;
		HandshakeCandidate( std::shared_ptr<Candidate> send, std::shared_ptr<Candidate> receive, double r, std::vector< int > i, std::vector< std::string > c ){

			hsSendCand = send;
			hsReceiveCand = receive;
			rate = r;
			receivedParam = i;
			channel = c;
		}
		std::vector< int > getReceivedParam(void){

			return receivedParam;
		}
};


class HandshakeChannel{

	private:
		std::vector< std::string > _channelName;
		GlobalVariables _globalVars;
		std::map< SystemProcess *, std::list< std::shared_ptr<Candidate> > > _hsSend_Sp2Candidates;
		std::map< SystemProcess *, std::list< std::shared_ptr<Candidate> > > _hsReceive_Sp2Candidates;
		std::map< SystemProcess *, std::list< std::shared_ptr<HandshakeCandidate> > > _possibleHandshakes_sp2Candidates;
		std::map< std::shared_ptr<HandshakeCandidate>, std::list< SystemProcess * > > _possibleHandshakes_candidates2Sp;
		std::list< std::shared_ptr<Candidate> > _sendToAdd;
		std::list< std::shared_ptr<Candidate> > _receiveToAdd;

	public:
		HandshakeChannel( std::vector< std::string > name, GlobalVariables & );
		HandshakeChannel( const HandshakeChannel & );
		std::vector< std::string > getChannelName(void);
		std::shared_ptr<HandshakeCandidate> buildHandshakeCandidate( std::shared_ptr<Candidate> , std::shared_ptr<Candidate> , std::vector<int> );
		std::pair<int, double> updateHandshakeCandidates(void);
		std::pair< int, double > cleanSPFromChannel( SystemProcess * );
		std::shared_ptr<HandshakeCandidate> pickCandidate(double &, double , double );
		void addSendCandidate( std::shared_ptr<Candidate> );
		void addReceiveCandidate( std::shared_ptr<Candidate> );
		bool matchClone (SystemProcess *, SystemProcess *);
		void cleanCloneFromChannel( SystemProcess *);
};

#endif

