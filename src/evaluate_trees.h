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

class NumericalOperand: public RPNoperand {

	private
		Numerical _underlyingNumerical;

	public:
		NumericalOperand( Numerical in ){_underlyingNumerical = in;}
		~NumericalOperand(){}
		std::string identify(void) const { return "Numerical"; };
		Numerical getValue(void){return _underlyingNumerical;}
}

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

class Numerical{

	private:
		double dVal;
		int iVal;
		bool isDouble = false;
		bool isInt = false;
	public:
		void setDouble(double d){

			assert(not isDouble and not isInt); //already set

			isDouble = true;
			isInt = false;
			dVal = d;
		}
		void setInt(int i){

			assert(not isDouble and not isInt); //already set

			isDouble = false;
			isInt = true;
			iVal = i;
		}
		int getInt(void){

			assert(isInt);
			return iVal;
		}
		double getDouble(void){

			assert(isDouble);
			return dVal;
		}
		double doubleCast(void){

			assert(not isDouble and not isInt); //already set
			if ( isDouble.() ) return getDouble();
			else return getInt();
		}
		bool isInt(void){

			assert(not isDouble and not isInt); //already set
			return isInt;
		}
		bool isDouble(void){

			assert(not isDouble and not isInt); //already set
			return isDouble;
		}
};


//int evalRPN_int( std::vector< Token * > , ParameterValues, GlobalVariables &, std::map< std::string, double > &);
//double evalRPN_double( std::vector< Token * > , ParameterValues, GlobalVariables &, std::map< std::string, double > &);
Numerical evalRPN_numerical( std::vector< Token * > , ParameterValues, GlobalVariables &, std::map< std::string, double > &);
bool evalRPN_condition( std::vector< Token * > , ParameterValues, GlobalVariables &, std::map< std::string, double > &);
bool evalRPN_set( int, std::vector< Token * > , ParameterValues, GlobalVariables &, std::map< std::string, double > &);
std::vector< Token * > shuntingYard( std::vector< Token * > &inputExp );
inline double substituteVariable( Token *, ParameterValues , GlobalVariables &, std::map< std::string, double > & );
inline bool variableIsDefined( Token *, ParameterValues, GlobalVariables &, std::map< std::string, double > &);
bool castToDouble( std::vector<Token * > , GlobalVariables , ParameterValues );

#endif
