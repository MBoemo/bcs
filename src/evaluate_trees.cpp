//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG_RPN 1

#include "evaluate_trees.h"
#include <cmath>
#include <stack>
#include <math.h>
#include <set>
#include <algorithm>


bool castToDouble( std::vector<Token * > expression, GlobalVariables gv, ParameterValues pv ){

	for ( auto t = expression.begin(); t < expression.end(); t++ ){

		if ( (*t) -> identify() == "DoubleLiteral" ) return true;
		if ( (*t) -> identify() == "Variable" ){

			if (gv.doubleValues.count((*t) -> value()) > 0) return true;
			if (pv.doubleValues.count((*t) -> value()) > 0) return true;
		}
	}
	return false;
}


bool isOperator( Token *t ){

	if ( not t ) return false;

	if( t -> identify() == "SetOperation" or t -> identify() == "Operator" or t -> identify() == "Comparison" ) return true;
	else return false;
}


bool isOperand( Token *t ){

	if ( not t ) return false;

	if( t -> identify() == "IntLiteral" or t -> identify() == "DoubleLiteral" or t -> identify() == "Variable" ) return true;
	else return false;
}


int precedence( Token *t ){

	std::map<std::string,int> precMap = {{"U", -4},
					    {"I", -3},
					    {"\\", -2},
					    {"..", -1},
					    {"&", 0},
					    {"|", 0},
					    {">", 1},
					    {"<", 1},
					    {">=", 1},
					    {"<=", 1},
					    {"==", 1},
					    {"!=", 1},
					    {"+", 2},
					    {"-", 2},
					    {"*", 3},
					    {"/", 3},
					    {"neg", 4},
					    {"^", 5}};
	assert( precMap.find( t-> value() ) != precMap.end() );

	return precMap[t->value()];
}



bool isLeftAsso( Token *t ){

	std::vector< std::string > leftAssoOps = {"*","/","+","-","U","I"};

	if (std::find( leftAssoOps.begin(), leftAssoOps.end(), t -> value() ) != leftAssoOps.end() ) return true;
	else return false;
}

std::vector< Token * > shuntingYard( std::vector< Token * > &inputExp ){

	std::stack < Token * > operatorStack;
	std::vector< Token * > expRPN;
	std::list< Token * > functionParen;
	Token *prevToken = NULL;

	for ( auto t = inputExp.begin(); t < inputExp.end(); t++ ){

		if ( (*t) -> identify() == "Comma" ){

			while( operatorStack.top() -> value() != "(" ){

				expRPN.push_back( operatorStack.top() );
				operatorStack.pop();
				if ( operatorStack.empty() ) throw UnbalancedParentheses(*t);
			}
			if ( std::find(functionParen.begin(), functionParen.end(), operatorStack.top()) == functionParen.end() ){

				throw UnbalancedParentheses(operatorStack.top());
			}
		}
		if ( isOperand(*t) ){

			expRPN.push_back( *t );
		}
		if ( (*t) -> identify() == "Function" ){

			operatorStack.push(*t); //push the function on the stack
			
			//get the next token, which should be an open paren
			t++;
			if ( (*t) -> value() != "(" ) throw SyntaxError(*t,"Token after function call should be left parentheses.");
			operatorStack.push(*t); //push the parentheses to the stack as normal
			functionParen.push_back(*t);
			continue;
		}
		if ( isOperator(*t) and (*t) -> value() != "-" ){ //any operator other than a minus sign

			while ( (not operatorStack.empty())
				and (operatorStack.top() -> identify() == "Function" 
			        or ( isOperator(operatorStack.top()) and precedence(operatorStack.top()) > precedence(*t) ) 
			        or ( isOperator(operatorStack.top()) and precedence(operatorStack.top()) == precedence(*t) and isLeftAsso(operatorStack.top()) ) )
				and ( ( operatorStack.top() -> value() != "(") ) ){

				expRPN.push_back( operatorStack.top() );
				operatorStack.pop();
			}
			operatorStack.push( *t );
		}
		if ( (*t) -> value() == "-" ){//minus signs need special handling because we need to figure out if it's subtraction (binary) or negation (unary)

			if (not isOperand(prevToken) ){ //the minus sign is a unary negative operator

				while ( (not operatorStack.empty())
					and (operatorStack.top() -> identify() == "Function" 
					or ( (*t) -> value() == "neg" ) ) //the only operators you can pop are other negative functions
					and ( ( operatorStack.top() -> value() != "(") ) ){

					expRPN.push_back( operatorStack.top() );
					operatorStack.pop();
				}
				operatorStack.push( new Token( (*t)->identify(), "neg", (*t)->getLine(), (*t)->getColumn() ) );
			}
			else{ //the minus sign is a binary subtraction operator

				while ( (not operatorStack.empty())
					and (operatorStack.top() -> identify() == "Function" 
					or ( isOperator(operatorStack.top()) and precedence(operatorStack.top()) > precedence(*t) ) 
					or ( isOperator(operatorStack.top()) and precedence(operatorStack.top()) == precedence(*t) and isLeftAsso(operatorStack.top()) ) )
					and ( ( operatorStack.top() -> value() != "(") ) ){

					expRPN.push_back( operatorStack.top() );
					operatorStack.pop();
				}
				operatorStack.push( *t );
			}
		}
		if ( (*t) -> value() == "(" ) operatorStack.push(*t);
		if ( (*t) -> value() == ")" ){

			while( operatorStack.top() -> value() != "(" ){

				expRPN.push_back( operatorStack.top() );
				operatorStack.pop();
				if ( operatorStack.empty() ) throw UnbalancedParentheses(*t);
			}
			if ( operatorStack.top() -> value() == "(" ) operatorStack.pop();
			else throw UnbalancedParentheses(*t);
		}
		prevToken = *t;

#if DEBUG_RPN
//print incrementally as we go
std::cout << "Queue: ";
for ( auto t = expRPN.begin(); t < expRPN.end(); t++ ){

	std::cout << (*t) -> value();
}
std::cout << std::endl;

std::cout << "Stack: ";
std::stack< Token * > copyStack = operatorStack;
while ( not copyStack.empty() ){

	std::cout << copyStack.top() -> value();
	copyStack.pop();
}
std::cout << std::endl;
std::cout << "-------" << std::endl;
#endif

	}
	while ( not operatorStack.empty() ){

		if ( operatorStack.top() -> value() == ")" or operatorStack.top() -> value() == "(" ) throw UnbalancedParentheses(operatorStack.top());
		if ( (not isOperator(operatorStack.top())) and (operatorStack.top() -> identify() != "Function") ){
			throw SyntaxError( operatorStack.top(), "Thrown by expression parser: Arithmetic, set, or comparison is incorrect." );
		}

		expRPN.push_back( operatorStack.top() );
		operatorStack.pop();		
	}

#if DEBUG_RPN
std::cout << "Whole expression in RPN: ";
	for ( auto t = expRPN.begin(); t < expRPN.end(); t++ ){

		std::cout << (*t) -> value();
	}
	std::cout << std::endl;
#endif

	return expRPN;
}


inline double substituteVariable( Token *t, ParameterValues param2value, GlobalVariables &globalVariables, std::map< std::string, double > &localVariables ){
//takes a variable token and looks for valid substitutions from the process's parameter values, the system's global variables, and local variables within the system process

	if ( t -> identify() == "DoubleLiteral" or t -> identify() == "IntLiteral" ){

		std::string litValue = t -> value();
		return atof( litValue.c_str() );
	}
	else{

		assert( t -> identify() == "Variable" );
		if ( localVariables.count( t -> value() ) > 0 ){

			return localVariables[ t -> value() ];
		}
		else if ( globalVariables.intValues.count( t -> value() ) > 0 ){

			return globalVariables.intValues[ t -> value() ];
		}
		else if ( globalVariables.doubleValues.count( t -> value() ) > 0 ){

			return globalVariables.doubleValues[ t -> value() ];
		}
		else if ( param2value.intValues.count( t -> value() ) > 0 ){

			return param2value.intValues[ t -> value() ];
		}
		else if ( param2value.doubleValues.count( t -> value() ) > 0 ){

			return param2value.doubleValues[ t -> value() ];
		}
		else throw UndefinedVariable( t );
	}
}


int evalRPN_int( std::vector< Token * > inputRPN, ParameterValues param2value, GlobalVariables &globalVariables, std::map< std::string, double > &localVariables){

	std::stack<RPNoperand *> evalStack;	

	for ( auto t = inputRPN.begin(); t < inputRPN.end(); t++ ){

		if ( isOperator(*t) or (*t) -> identify() == "Function"){

			if ( (*t) -> value() == "abs" or (*t) -> value() == "sqrt" or (*t) ->value() == "neg" ){ //unary

				if ( evalStack.size() < 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand = evalStack.top();
				evalStack.pop();
				if (operand -> identify() != "Int") throw WrongType(*t,operand -> identify());
				IntOperand *intptr = dynamic_cast<IntOperand *>(operand);
				int op_i = intptr -> getValue();
				int result;
				if ( (*t) -> value() == "abs" ){
					result = std::abs(op_i);
				}
				else if ( (*t) -> value() == "sqrt" ){
					result = sqrt(op_i);
				}
				else if ( (*t) -> value() == "neg" ){
					result = -op_i;
				}
				else assert(false);

				evalStack.push( new IntOperand(result) );
				delete operand;
			}
			else if ( (*t) -> value() == "min" or (*t) -> value() == "max" or (*t) -> value() == "+" or (*t) -> value() == "-" or (*t) -> value() == "*" or (*t) -> value() == "/" or (*t) -> value() == "^"){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Int") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Int") throw WrongType(*t,operand2 -> identify());
				int op1_d,op2_d;
				IntOperand *intptr = dynamic_cast<IntOperand *>(operand1);
				op1_d = intptr -> getValue();
				intptr = dynamic_cast<IntOperand *>(operand2);
				op2_d = intptr -> getValue();

				int result;
				if ( (*t) -> value() == "+" ){
					result = op1_d + op2_d;
				}
				else if ( (*t) -> value() == "-" ){
					result = op1_d - op2_d;
				}
				else if ( (*t) -> value() == "/" ){
					result = op1_d / op2_d;
				}
				else if ( (*t) -> value() == "*" ){
					result = op1_d * op2_d;
				}
				else if ( (*t) -> value() == "^" ){
					result = pow(op1_d, op2_d);
				}
				else if ( (*t) -> value() == "min" ){
					result = std::min(op1_d, op2_d);
				}
				else if ( (*t) -> value() == "max" ){
					result = std::max(op1_d, op2_d);
				}
				else assert(false);

				evalStack.push( new IntOperand(result) );
				delete operand1; delete operand2;
			}

		}
		else if ( isOperand(*t) ){

			//check for casting issues
			if ( (*t) -> identify() == "DoubleLiteral" ) throw WrongType(*t, "Operands in gate, message, and set expressions must be ints.");
			if ( (*t) -> identify() == "Variable" ){

				if ( globalVariables.doubleValues.count((*t) -> value()) > 0 or param2value.doubleValues.count((*t) -> value()) > 0 ){

					throw WrongType(*t, "Operands in gate, message, and set expressions must be ints.");
				}
			}

			int result = substituteVariable( *t, param2value, globalVariables, localVariables );
			evalStack.push( new IntOperand(result) );
		}
	}

	if ( evalStack.top() -> identify() != "Int" ) throw SyntaxError( inputRPN[0], "Message send expression must evaluate to an int." );

	IntOperand *intptr = dynamic_cast<IntOperand *>(evalStack.top());
	int result = intptr -> getValue();
	delete evalStack.top();

#if DEBUG_RPN
	std::cout << "Int is: " << result << std::endl;
#endif
	return result;
}


double evalRPN_double( std::vector< Token * > inputRPN, ParameterValues param2value, GlobalVariables &globalVariables, std::map< std::string, double > &localVariables){

	std::stack<RPNoperand *> evalStack;	

	for ( auto t = inputRPN.begin(); t < inputRPN.end(); t++ ){

		if ( isOperator(*t) or (*t) -> identify() == "Function"){

			if ( (*t) -> value() == "abs" or (*t) -> value() == "sqrt" or (*t)->value() == "neg" ){

				if ( evalStack.size() < 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand = evalStack.top();
				evalStack.pop();
				if (operand -> identify() != "Double") throw WrongType(*t,operand -> identify());
				DoubleOperand *intptr = dynamic_cast<DoubleOperand *>(operand);
				double op_d = intptr -> getValue();
				double result;
				if ( (*t) -> value() == "abs" ){
					result = std::abs(op_d);
				}
				else if ( (*t) -> value() == "sqrt" ){
					result = sqrt(op_d);
				}
				else if ( (*t) -> value() == "neg" ){
					result = -op_d;
				}
				else assert(false);

				evalStack.push( new DoubleOperand(result) );
				delete operand;
			}
			else if ( (*t) -> value() == "min" or (*t) -> value() == "max" or (*t) -> value() == "+" or (*t) -> value() == "-" or (*t) -> value() == "*" or (*t) -> value() == "/" or (*t) -> value() == "^"){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Double") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Double") throw WrongType(*t,operand2 -> identify());
				double op1_d,op2_d;
				DoubleOperand *doubleptr = dynamic_cast<DoubleOperand *>(operand1);
				op1_d = doubleptr -> getValue();
				doubleptr = dynamic_cast<DoubleOperand *>(operand2);
				op2_d = doubleptr -> getValue();

				double result;
				if ( (*t) -> value() == "+" ){
					result = op1_d + op2_d;
				}
				else if ( (*t) -> value() == "-" ){
					result = op1_d - op2_d;
				}
				else if ( (*t) -> value() == "/" ){
					result = op1_d / op2_d;
				}
				else if ( (*t) -> value() == "*" ){
					result = op1_d * op2_d;
				}
				else if ( (*t) -> value() == "^" ){
					result = pow(op1_d, op2_d);
				}
				else if ( (*t) -> value() == "min" ){
					result = std::min(op1_d, op2_d);
				}
				else if ( (*t) -> value() == "max" ){
					result = std::max(op1_d, op2_d);
				}
				else assert(false);

				evalStack.push( new DoubleOperand(result) );
				delete operand1; delete operand2;
			}
		}
		else if ( isOperand(*t) ){

			double result = substituteVariable( *t, param2value, globalVariables, localVariables );
			evalStack.push( new DoubleOperand(result) );
		}
	}

	if ( evalStack.top() -> identify() != "Double" ) throw SyntaxError( inputRPN[0], "Rate expression must evaluate to a double." );

	double result;
	DoubleOperand *douptr = dynamic_cast<DoubleOperand *>(evalStack.top());
	result = douptr -> getValue();
	delete evalStack.top();

#if DEBUG_RPN
	std::cout << "Double is: " << result << std::endl;
#endif
	return result;
}


bool evalRPN_condition( std::vector< Token * > inputRPN, ParameterValues param2value, GlobalVariables &globalVariables, std::map< std::string, double > &localVariables){

	std::stack<RPNoperand *> evalStack;	

	for ( auto t = inputRPN.begin(); t < inputRPN.end(); t++ ){

		if ( isOperator(*t) or (*t) -> identify() == "Function"){

			if ( (*t) -> value() == "abs" or (*t) -> value() == "sqrt" or (*t) -> value() == "neg" ){

				if ( evalStack.size() < 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand = evalStack.top();
				evalStack.pop();
				if (operand -> identify() != "Int") throw WrongType(*t,operand -> identify());
				IntOperand *intptr = dynamic_cast<IntOperand *>(operand);
				int op_d = intptr -> getValue();
				int result;
				if ( (*t) -> value() == "abs" ){
					result = std::abs(op_d);
				}
				else if ( (*t) -> value() == "sqrt" ){
					result = sqrt(op_d);
				}
				else if ( (*t) -> value() == "neg" ){
					result = -op_d;
				}
				else assert(false);

				evalStack.push( new IntOperand(result) );
				delete operand;
			}
			else if ( (*t) -> value() == "min" or (*t) -> value() == "max" or (*t) -> value() == "+" or (*t) -> value() == "-" or (*t) -> value() == "*" or (*t) -> value() == "/" or (*t) -> value() == "^"){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Int") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Int") throw WrongType(*t,operand2 -> identify());
				int op1_d,op2_d;
				IntOperand *intptr = dynamic_cast<IntOperand *>(operand1);
				op1_d = intptr -> getValue();
				intptr = dynamic_cast<IntOperand *>(operand2);
				op2_d = intptr -> getValue();

				int result;
				if ( (*t) -> value() == "+" ){
					result = op1_d + op2_d;
				}
				else if ( (*t) -> value() == "-" ){
					result = op1_d - op2_d;
				}
				else if ( (*t) -> value() == "/" ){
					result = op1_d / op2_d;
				}
				else if ( (*t) -> value() == "*" ){
					result = op1_d * op2_d;
				}
				else if ( (*t) -> value() == "^" ){
					result = pow(op1_d, op2_d);
				}
				else if ( (*t) -> value() == "min" ){
					result = std::min(op1_d, op2_d);
				}
				else if ( (*t) -> value() == "max" ){
					result = std::max(op1_d, op2_d);
				}
				else assert(false);

				evalStack.push( new IntOperand(result) );
				delete operand1; delete operand2;
			}
			else if ( (*t) -> value() == "==" or (*t) -> value() == "!=" or (*t) -> value() == ">" or (*t) -> value() == "<" or (*t) -> value() == ">=" or (*t) -> value() == "<=" ){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Int") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Int") throw WrongType(*t,operand2 -> identify());
				int op1_d,op2_d;
				IntOperand *intptr = dynamic_cast<IntOperand *>(operand1);
				op1_d = intptr -> getValue();
				intptr = dynamic_cast<IntOperand *>(operand2);
				op2_d = intptr -> getValue();

				bool result;
				if ( (*t) -> value() == "==" ){
					result = op1_d == op2_d;
				}
				else if ( (*t) -> value() == "!=" ){
					result = op1_d != op2_d;
				}
				else if ( (*t) -> value() == ">" ){
					result = op1_d > op2_d;
				}
				else if ( (*t) -> value() == "<" ){
					result = op1_d < op2_d;
				}
				else if ( (*t) -> value() == ">=" ){
					result = op1_d >= op2_d;
				}
				else if ( (*t) -> value() == "<=" ){
					result = op1_d <= op2_d;
				}
				else assert(false);

				evalStack.push( new BoolOperand(result) );
				delete operand1; delete operand2;
			}
			else if ( (*t) -> value() == "|" or (*t) -> value() == "&" ){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Bool") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Bool") throw WrongType(*t,operand2 -> identify());
				bool op1_d,op2_d;
				BoolOperand *boolptr = dynamic_cast<BoolOperand *>(operand1);
				op1_d = boolptr -> getValue();
				boolptr = dynamic_cast<BoolOperand *>(operand2);
				op2_d = boolptr -> getValue();

				bool result;
				if ( (*t) -> value() == "|" ){
					result = op1_d or op2_d;
				}
				else if ( (*t) -> value() == "&" ){
					result = op1_d and op2_d;
				}
				else assert(false);

				evalStack.push( new BoolOperand(result) );
				delete operand1; delete operand2;
			}
			
		}
		else if ( isOperand(*t) ){

			//check for casting issues
			if ( (*t) -> identify() == "DoubleLiteral" ) throw WrongType(*t, "Operands to gate conditions must be ints.");
			if ( (*t) -> identify() == "Variable" ){

				if ( globalVariables.doubleValues.count((*t) -> value()) > 0 or param2value.doubleValues.count((*t) -> value()) > 0 ){

					throw WrongType(*t, "Operands to gate conditions must be ints.");
				}
			}

			int result = substituteVariable( *t, param2value, globalVariables, localVariables );
			evalStack.push( new IntOperand(result) );
		}
	}

	if ( evalStack.top() -> identify() != "Bool" ) throw SyntaxError( inputRPN[0], "Gate expression must evaluate to a bool." );

	bool result;
	BoolOperand *boolptr = dynamic_cast<BoolOperand *>(evalStack.top());
	result = boolptr -> getValue();

	delete evalStack.top();

#if DEBUG_RPN
	std::cout << "Bool is: " << result << std::endl;
#endif
	return result;
}


std::set<int> evalRPN_set( std::vector< Token * > inputRPN, ParameterValues param2value, GlobalVariables &globalVariables, std::map< std::string, double > &localVariables){

	std::stack<RPNoperand *> evalStack;	

	for ( auto t = inputRPN.begin(); t < inputRPN.end(); t++ ){

		if ( isOperator(*t) or (*t) -> identify() == "Function"){

			if ( (*t) -> value() == "abs" or (*t) -> value() == "sqrt" or (*t) ->  value() == "neg"){

				if ( evalStack.size() < 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand = evalStack.top();
				evalStack.pop();
				if (operand -> identify() != "Int") throw WrongType(*t,operand -> identify());
				IntOperand *intptr = dynamic_cast<IntOperand *>(operand);
				int op_d = intptr -> getValue();
				int result;
				if ( (*t) -> value() == "abs" ){
					result = std::abs(op_d);
				}
				else if ( (*t) -> value() == "sqrt" ){
					result = sqrt(op_d);
				}
				else if ( (*t) -> value() == "neg" ){
					result = -op_d;
				}
				else assert(false);

				evalStack.push( new IntOperand(result) );
				delete operand;
			}
			else if ( (*t) -> value() == "min" or (*t) -> value() == "max" or (*t) -> value() == "+" or (*t) -> value() == "-" or (*t) -> value() == "*" or (*t) -> value() == "/" or (*t) -> value() == "^"){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Int") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Int") throw WrongType(*t,operand2 -> identify());
				int op1_d,op2_d;
				IntOperand *intptr = dynamic_cast<IntOperand *>(operand1);
				op1_d = intptr -> getValue();
				intptr = dynamic_cast<IntOperand *>(operand2);
				op2_d = intptr -> getValue();

				int result;
				if ( (*t) -> value() == "+" ){
					result = op1_d + op2_d;
				}
				else if ( (*t) -> value() == "-" ){
					result = op1_d - op2_d;
				}
				else if ( (*t) -> value() == "/" ){
					result = op1_d / op2_d;
				}
				else if ( (*t) -> value() == "*" ){
					result = op1_d * op2_d;
				}
				else if ( (*t) -> value() == "^" ){
					result = pow(op1_d, op2_d);
				}
				else if ( (*t) -> value() == "min" ){
					result = std::min(op1_d, op2_d);
				}
				else if ( (*t) -> value() == "max" ){
					result = std::max(op1_d, op2_d);
				}
				else assert(false);

				evalStack.push( new IntOperand(result) );
				delete operand1; delete operand2;
			}
			else if ( (*t) -> value() == ".." ){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Int") throw WrongType(*t,operand1 -> identify());
				if (operand1 -> identify() != "Int") throw WrongType(*t,operand2 -> identify());
				int op1_d,op2_d;
				IntOperand *intptr = dynamic_cast<IntOperand *>(operand1);
				op1_d = intptr -> getValue();
				intptr = dynamic_cast<IntOperand *>(operand2);
				op2_d = intptr -> getValue();

				std::set<int> result;
				if (op1_d > op2_d ) throw SyntaxError(*t,"Thrown by expression evaluation (sets).  Range upper bound is greater than range lower bound.");
				for ( int i = op1_d; i <= op2_d; i++ ) result.insert(i);

				evalStack.push( new SetOperand(result) );
				delete operand1; delete operand2;
			}
			else if ( (*t) -> value() == "U" or (*t) -> value() == "I" or (*t) -> value() == "\\" ){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Int" and operand1 -> identify() != "Set") throw WrongType(*t,operand1 -> identify());
				if (operand1 -> identify() != "Int" and operand1 -> identify() != "Set") throw WrongType(*t,operand2 -> identify());
				
				//get everything in set format
				std::set<int> op1_s, op2_s;
				if (operand1 -> identify() == "Int"){
					IntOperand *intptr = dynamic_cast<IntOperand *>(operand1);
					op1_s.insert( intptr -> getValue() );
				}
				else{
					SetOperand *setptr = dynamic_cast<SetOperand *>(operand1);
					op1_s = setptr -> getValue();
				}
				if (operand2 -> identify() == "Int"){
					IntOperand *intptr = dynamic_cast<IntOperand *>(operand2);
					op2_s.insert( intptr -> getValue() );
				}
				else{
					SetOperand *setptr = dynamic_cast<SetOperand *>(operand2);
					op2_s = setptr -> getValue();
				}

				std::set<int> result;
				if ( (*t) -> value() == "U" ){
					std::set_union(op1_s.begin(), op1_s.end(), op2_s.begin(), op2_s.end(), std::inserter(result, result.begin()));
				}
				else if ( (*t) -> value() == "I" ){
					std::set_intersection(op1_s.begin(), op1_s.end(), op2_s.begin(), op2_s.end(), std::inserter(result, result.begin()));
				}
				else if ( (*t) -> value() == "\\" ){
					std::set_difference(op1_s.begin(), op1_s.end(), op2_s.begin(), op2_s.end(), std::inserter(result, result.begin()));
				}

				evalStack.push( new SetOperand(result) );
				delete operand1; delete operand2;
			}
		}
		else if ( isOperand(*t) ){

			//check for casting issues
			if ( (*t) -> identify() == "DoubleLiteral" ) throw WrongType(*t, "Operands within message receives must be ints.");
			if ( (*t) -> identify() == "Variable" ){

				if ( globalVariables.doubleValues.count((*t) -> value()) > 0 or param2value.doubleValues.count((*t) -> value()) > 0 ){

					throw WrongType(*t, "Operands within message receives must be ints.");
				}
			}

			int result = substituteVariable( *t, param2value, globalVariables, localVariables );
			evalStack.push( new IntOperand(result) );
		}
	}

	if ( evalStack.top() -> identify() != "Set" and evalStack.top() -> identify() != "Int" ) throw SyntaxError( inputRPN[0], "Message receive expression must evaluate to a set or an int" );

	std::set<int> result;
	if ( evalStack.top() -> identify() == "Int" ){

		IntOperand *intptr = dynamic_cast<IntOperand *>(evalStack.top());
		result.insert(intptr -> getValue());
	}
	else{
		SetOperand *setptr = dynamic_cast<SetOperand *>(evalStack.top());
		result = setptr -> getValue();
	}
	delete evalStack.top();

#if DEBUG_RPN
	for (auto i = result.begin(); i != result.end(); i++) std::cout << "Set contains: " << *i << std::endl;
#endif
	return result;
}
