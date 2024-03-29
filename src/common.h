//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef COMMON_H
#define COMMON_H

#define VERSION "1.1.0"

#include "blockParser.h"
#include <memory>

bool compareSystemProcesses(const SystemProcess &sp1, const SystemProcess &sp2);


struct findSp{

	findSp( SystemProcess *toCompare) : compareAgainst {toCompare} {}
	bool operator()( SystemProcess *toCompare){

		for (auto i = compareAgainst.begin(); i != compareAgainst.end(); i++){

			if (compareSystemProcesses(*toCompare,**i)) return true;
		}
		return false;
	}
	std::list<SystemProcess *> compareAgainst;
};

bool compareCandidates( std::shared_ptr<Candidate> &c1, std::shared_ptr<Candidate> &c2 );

#endif
