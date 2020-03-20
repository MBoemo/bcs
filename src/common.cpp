//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#include "common.h"

bool compareCandidates( std::shared_ptr<Candidate> &c1, std::shared_ptr<Candidate> &c2 ){

	//candidates match if they perform the same action block and have the same parallel process (e.g., same process -> process history)
	if (c1 -> actionCandidate != c2 -> actionCandidate) return false;
	bool processesMatch = std::is_permutation((c1 -> parallelProcesses).begin(), (c1 -> parallelProcesses).end(), (c2 -> parallelProcesses).begin());
	return processesMatch;
}
