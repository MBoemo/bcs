//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#include "numerical.h"


bool operator== (const Numerical &n1, const Numerical &n2){

	if (n1.isDouble_b && n2.isDouble_b){

		if (n1.dVal == n2.dVal) return true;
		else return false;
	}
	else if (n1.isInt_b && n2.isInt_b){

		if (n1.iVal == n2.iVal) return true;
		else return false;
	}
	else return false;
}

bool operator!= (const Numerical &n1, const Numerical &n2){

	return !(n1 == n2);
}
