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
	bool processesMatch = std::is_permutation((c1 -> parallelProcesses).begin(), (c1 -> parallelProcesses).end(), (c2 -> parallelProcesses).begin(),compareSystemProcesses);
	return processesMatch;
}

bool compareSystemProcesses(const SystemProcess &sp1, const SystemProcess &sp2){

	//fast exit if obviously not a match
	if (sp2.parameterValues.getSize() !=  sp1.parameterValues.getSize()) return false;
	if (sp2.localVariables.size() !=  sp1.localVariables.size()) return false;
	if (sp2.parseTree.getRoot() !=  sp1.parseTree.getRoot()) return false;

	if (sp1.parseTree == sp2.parseTree && sp1.parameterValues == sp2.parameterValues && sp1.localVariables == sp2.localVariables) return true;
	else return false;
}
