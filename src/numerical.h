//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef NUMERICAL_H
#define NUMERICAL_H

#include <limits>

class Numerical{

	private:
		double dVal;
		int iVal;
		bool isDouble_b;
		bool isInt_b;

	public:
		Numerical(void){
			dVal = std::numeric_limits<double>::min();
			iVal = std::numeric_limits<int>::min();
			isDouble_b = false;
			isInt_b = false;
		}
		Numerical(const Numerical &n){

			dVal = n.dVal;
			iVal = n.iVal;
			isDouble_b = n.isDouble_b;
			isInt_b = n.isInt_b;
		}
		inline void setDouble(double d){

			assert(not isDouble_b and not isInt_b); //not already set
			isDouble_b = true;
			isInt_b = false;
			dVal = d;
		}
		inline void setInt(int i){

			assert(not isDouble_b and not isInt_b); //not already set
			isDouble_b = false;
			isInt_b = true;
			iVal = i;
		}
		inline int getInt(void){

			assert(isInt_b and not isDouble_b);
			return iVal;
		}
		inline double getDouble(void){

			assert(isDouble_b and not isInt_b);
			return dVal;
		}
		inline double doubleCast(void){

			assert(isDouble_b or isInt_b); //is already set
			if ( isDouble_b ) return getDouble();
			else return getInt();
		}
		inline bool isInt(void){

			assert(isDouble_b or isInt_b); //is already set
			return isInt_b;
		}
		inline bool isDouble(void){

			assert(isDouble_b or isInt_b); //is already set
			return isDouble_b;
		}
};

#endif
