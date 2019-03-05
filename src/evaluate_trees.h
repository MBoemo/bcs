//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef EVALUATE_TREES_H
#define EVALUATE_TREES_H

#include "blockParser.h"
#include <set>

class RPNoperand{

	public:
		virtual std::string identify( void ) const = 0;
		virtual ~RPNoperand(){}

};

class IntOperand: public RPNoperand {

	private:
		int _underlyingInt;

	public:
		IntOperand( double in ){_underlyingInt = in;}
 		~IntOperand(){}
		std::string identify( void ) const { return "Int"; }
		double getValue(void){return _underlyingInt;}
};

class DoubleOperand: public RPNoperand {

	private:
		double _underlyingDouble;

	public:
		DoubleOperand( double in ){_underlyingDouble = in;}
 		~DoubleOperand(){}
		std::string identify( void ) const { return "Double"; }
		double getValue(void){return _underlyingDouble;}
};

class BoolOperand: public RPNoperand {

	private:
		bool _underlyingBool;

	public:
		BoolOperand( bool in ){_underlyingBool = in;}
 		~BoolOperand(){}
		std::string identify( void ) const { return "Bool"; }
		bool getValue(void){return _underlyingBool;}
};

class SetOperand: public RPNoperand {

	private:
		std::set<int> _underlyingSet;

	public:
		SetOperand( std::set<int> in ){_underlyingSet = in;}
 		~SetOperand(){}
		std::string identify( void ) const { return "Set"; }
		std::set<int> getValue(void){return _underlyingSet;}
};


int evalRPN_int( std::vector< Token * > , ParameterValues, GlobalVariables &, std::map< std::string, double > &);
double evalRPN_double( std::vector< Token * > , ParameterValues, GlobalVariables &, std::map< std::string, double > &);
bool evalRPN_condition( std::vector< Token * > , ParameterValues, GlobalVariables &, std::map< std::string, double > &);
std::set<int> evalRPN_set( std::vector< Token * > , ParameterValues, GlobalVariables &, std::map< std::string, double > &);
std::vector< Token * > shuntingYard( std::vector< Token * > &inputExp );
inline double substituteVariable( Token *, ParameterValues , GlobalVariables &, std::map< std::string, double > & );
bool castToDouble( std::vector<Token * > , GlobalVariables , ParameterValues );

#endif
