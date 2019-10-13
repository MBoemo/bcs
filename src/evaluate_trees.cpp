//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG_RPN 1
//#define DEBUG_GRAMMAR 1
//#define DEBUG_SETS 1

#include "evaluate_trees.h"
#include <cmath>
#include <stack>
#include <math.h>
#include <set>
#include <algorithm>


bool castToDouble( std::vector<Token * > expression, GlobalVariables &gv, ParameterValues &pv ){

	for ( auto t = expression.begin(); t < expression.end(); t++ ){

		if ( (*t) -> identify() == "DoubleLiteral" ) return true;
		if ( (*t) -> identify() == "Variable" ){

			if (gv.values.count((*t) -> value()) > 0) return true;
			if (pv.values.count((*t) -> value()) > 0) return true;
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


int parsePrecedence( Token *t ){

	std::map<std::string,int> precMap = {{"min",-5},
					    {"max",-5},
					    {"sqrt",-5},
					    {"abs",-5},
					    {"U", -4},
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


bool isValidInfixExpression( std::vector< Token * > inputExp ){

#if DEBUG_GRAMMAR
	std::cout << "----------------------------" << std::endl;
	std::cout << "Number of tokens in expression: " << inputExp.size() << std::endl;
	std::cout << "Tokenised expression: ";
	for ( auto t = inputExp.begin();t<inputExp.end();t++) std::cout << (*t) -> value() << " ";
	std::cout << std::endl;
#endif

	//if this expression is flanked on the ends by matching parentheses, get rid of them
	if ( inputExp[0] -> value() == "(" and inputExp.back() -> value() == ")"){

		//make sure these flanking parentheses close each other
		int match = 1;
		for ( unsigned int j = 1; j < inputExp.size(); j++ ){

			if ( inputExp[j] -> value() == "(" ) match++;
			else if ( inputExp[j] -> value() == ")" ) match--;

			//if this expression is totally enclosed by matching parentheses, get rid of them
			if ( match == 0 and j == inputExp.size()-1 ){

				inputExp.erase( inputExp.begin() );
				inputExp.erase( inputExp.end() - 1 );
				break;
			}
			else if ( match == 0 and j != inputExp.size()-1 ) break;
		}
	}

	//terminate if we've got down to a single value
	if (inputExp.size() == 1){

		if (inputExp[0] -> identify() == "Variable" or inputExp[0] -> identify() == "IntLiteral" or inputExp[0] -> identify() == "DoubleLiteral") return true;
		else return false;
	}

	/*vector for tokens for right hand side and left hand side for the pivot */
	std::vector< Token * > LHS, RHS;
	std::stack< Token * > parenStack;

	//find operator with the highest precedence
	int highScore = -6;
	std::vector<Token *>::iterator idx;
	bool found = false;
	for ( auto t = inputExp.begin(); t < inputExp.end(); t++ ){

		if ( (*t) -> value() == "(" ) parenStack.push( *t );
		else if ( (*t) -> value() == ")" ) parenStack.pop();

		if ( (isOperator(*t) or (*t) -> identify() == "Function") and parenStack.empty() ){

			int precedence;
			if ((*t) -> value() == "-" and (t == inputExp.begin() or isOperator(*t) or (*t) -> value() == "(")){//is negation

				precedence = 4;
			}
			else precedence = parsePrecedence(*t);
			
			if (precedence > highScore){

				highScore = precedence;
				idx = t;
				found = true;
			}
		}
	}
	if ( not parenStack.empty() ) throw UnbalancedParentheses(inputExp[0]);
	if ( not found ) return false;

	LHS.insert(LHS.end(), inputExp.begin(), idx);
	RHS.insert(RHS.end(), idx+1, inputExp.end());

	std::vector< std::string > binaryOperators = {"U","I","\\","..","&","|",">","<",">=","<=","==","!=","+","*","/","^"};
	if ( std::find(binaryOperators.begin(),binaryOperators.end(),(*idx) -> value()) != binaryOperators.end()  ){

		if (LHS.size() == 0 or RHS.size() == 0) return false;
		if (isValidInfixExpression(LHS) and isValidInfixExpression(RHS)) return true;
		else return false;
	}
	else if ((*idx) -> value() == "-" and idx == inputExp.begin()){//negation

		inputExp.erase(idx);
		if (isValidInfixExpression(inputExp)) return true;
		else return false;
	}
	else if ((*idx) -> value() == "-" and (isOperator(*(idx-1)) or (*(idx-1)) -> value() == "(")){//negation

		inputExp.erase(idx);
		if (isValidInfixExpression(inputExp)) return true;
		else return false;
	}
	else if ((*idx) -> value() == "-"){ //subtraction

		if (LHS.size() == 0 or RHS.size() == 0) return false;
		if (isValidInfixExpression(LHS) and isValidInfixExpression(RHS)) return true;
		else return false;
	}
	else if ((*idx) -> value() == "min" or (*idx) -> value() == "max"){ //min or max functions

		if (RHS.size() < 5 or LHS.size() != 0) return false;
		if ( (*(idx+1)) -> value() != "(" or (*(inputExp.end()-1)) -> value() != ")" ) throw SyntaxError(*idx,"Arguments to function call should be surrounded by parentheses.");

		//erase the function call and the parentheses
		inputExp.erase( inputExp.begin() +1 ); //erase (
		inputExp.erase( inputExp.begin() ); //erase min or max
		inputExp.erase( inputExp.end() - 1 ); //erase )

		//find the comma
		std::stack< Token * > commaStack;
		std::vector<Token *>::iterator commaidx;
		bool foundComma = false;
		for ( auto subidx = inputExp.begin(); subidx < inputExp.end(); subidx++ ){

			if ( (*subidx) -> value() == "(" ) commaStack.push( *subidx );
			else if ( (*subidx) -> value() == ")" ) commaStack.pop();
			if (commaStack.empty() and (*subidx) -> identify() == "Comma"){
			
				commaidx = subidx;
				foundComma = true;
				break;
			}
		}
		if (not foundComma) return false;

		std::vector< Token * > firstArg, secondArg;
		firstArg.insert(firstArg.end(), inputExp.begin(), commaidx);
		secondArg.insert(secondArg.end(), commaidx+1, inputExp.end());

		if (firstArg.size() == 0 or secondArg.size() == 0) throw SyntaxError(inputExp[0], "Function call argument is empty.");
		if (isValidInfixExpression(firstArg) and isValidInfixExpression(secondArg)) return true;
		else return false;
	}
	else if ((*idx) -> value() == "abs" or (*idx) -> value() == "sqrt"){ //sqrt or abs functions
	
		if (RHS.size() < 3 or LHS.size() != 0) return false;
		if ( (*(idx+1)) -> value() != "(" or (*(inputExp.end()-1)) -> value() != ")" ) throw SyntaxError(*idx,"Arguments to function call should be surrounded by parentheses.");

		//erase the function call and the parentheses
		inputExp.erase( inputExp.begin() +1 ); //erase (
		inputExp.erase( inputExp.begin() ); //erase abs or sqrt
		inputExp.erase( inputExp.end() - 1 ); //erase )

		if (isValidInfixExpression(inputExp)) return true;
		else return false;
	}
	else return false;
}


std::vector< Token * > shuntingYard( std::vector< Token * > &inputExp ){

	//grammar check
	if (not isValidInfixExpression(inputExp)) throw SyntaxError(inputExp[0], "Thrown by expression parser: Malformed infix expression.");

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
			if ( t == inputExp.end() ) throw SyntaxError(*(t-1),"Token after function call should be left parentheses.");
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
				operatorStack.push( new Token( (*t) -> identify(), "neg", (*t) -> getLine(), (*t) -> getColumn() ) );
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

	std::cout << (*t) -> value() << " (" << (*t) -> identify() << ") ";
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
			throw SyntaxError( operatorStack.top(), "Thrown by expression parser: Expression is incorrectly formatted." );
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


inline Numerical substituteVariable( Token *t, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables ){
//takes a variable token and looks for valid substitutions from the process's parameter values, the system's global variables, and local variables within the system process

	Numerical out;
	if ( t -> identify() == "DoubleLiteral" ){

		std::string litValue = t -> value();
		out.setDouble(atof(litValue.c_str()));
		return out;
	}
	else if ( t -> identify() == "IntLiteral" ){

		std::string litValue = t -> value();
		out.setInt(atoi(litValue.c_str()));
		return out;
	}
	else{

		assert( t -> identify() == "Variable" );
		if ( localVariables.count( t -> value() ) > 0 ){

			return localVariables[ t -> value() ];
		}
		else if ( globalVariables.values.count( t -> value() ) > 0 ){

			return globalVariables.values[ t -> value() ];
		}
		else if ( param2value.values.count( t -> value() ) > 0 ){

			return param2value.values[ t -> value() ];
		}
		else throw UndefinedVariable( t );
	}
}


inline bool variableIsDefined( Token *t, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables ){
//takes a variable token and looks for valid substitutions from the process's parameter values, the system's global variables, and local variables within the system process

	assert( t -> identify() == "Variable" );
	if ( localVariables.count( t -> value() ) > 0 ){

		return true;
	}
	else if ( globalVariables.values.count( t -> value() ) > 0 ){

		return true;
	}
	else if ( param2value.values.count( t -> value() ) > 0 ){

		return true;
	}
	else return false;
}


Numerical evalRPN_numerical( std::vector< Token * > inputRPN, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

	std::stack<RPNoperand *> evalStack;	

	for ( auto t = inputRPN.begin(); t < inputRPN.end(); t++ ){

		if ( isOperator(*t) or (*t) -> identify() == "Function"){

			if ( (*t) -> value() == "abs" or (*t) -> value() == "sqrt" or (*t) ->value() == "neg" ){ //unary

				if ( evalStack.size() < 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand = evalStack.top();
				evalStack.pop();
				if (operand -> identify() != "Numerical") throw WrongType(*t,operand -> identify());
				NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand);
				Numerical op_n = numptr -> getValue();
				Numerical result;
				if ( (*t) -> value() == "abs" ){

					if (op_n.isInt()) result.setInt(std::abs(op_n.getInt()));
					else result.setDouble(std::abs(op_n.getDouble()));
				}
				else if ( (*t) -> value() == "sqrt" ){

					if (op_n.isInt()) result.setInt(sqrt(op_n.getInt()));
					else result.setDouble(sqrt(op_n.getDouble()));
				}
				else if ( (*t) -> value() == "neg" ){

					if (op_n.isInt()) result.setInt(-op_n.getInt());
					else result.setDouble(-op_n.getDouble());
				}
				else throw SyntaxError(*t, "Unrecognised operator for function evaluation.");

				evalStack.push( new NumericalOperand(result) );
				delete operand;
			}
			else if ( (*t) -> value() == "min" or (*t) -> value() == "max" or (*t) -> value() == "+" or (*t) -> value() == "-" or (*t) -> value() == "*" or (*t) -> value() == "/" or (*t) -> value() == "^"){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Numerical") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Numerical") throw WrongType(*t,operand2 -> identify());
				Numerical op1_n,op2_n;
				NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand1);
				op1_n = numptr -> getValue();
				numptr = dynamic_cast<NumericalOperand *>(operand2);
				op2_n = numptr -> getValue();

				//upcast
				bool upcast = false;
				if (op1_n.isDouble() or op2_n.isDouble()) upcast = true;

				Numerical result;
				if ( (*t) -> value() == "+" ){

					if (upcast) result.setDouble( op1_n.doubleCast() + op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() + op2_n.getInt() );
				}
				else if ( (*t) -> value() == "-" ){

					if (upcast) result.setDouble( op1_n.doubleCast() - op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() - op2_n.getInt() );
				}
				else if ( (*t) -> value() == "/" ){

					if (upcast) result.setDouble( op1_n.doubleCast() / op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() / op2_n.getInt() );
				}
				else if ( (*t) -> value() == "*" ){
					if (upcast) result.setDouble( op1_n.doubleCast() * op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() * op2_n.getInt() );
				}
				else if ( (*t) -> value() == "^" ){

					if (upcast) result.setDouble( pow(op1_n.doubleCast(), op2_n.doubleCast()) );
					else result.setInt( pow(op1_n.getInt(), op2_n.getInt()) );
				}
				else if ( (*t) -> value() == "min" ){

					if (upcast) result.setDouble( std::min(op1_n.doubleCast(), op2_n.doubleCast()) );
					else result.setInt( std::min(op1_n.getInt(), op2_n.getInt()) );
				}
				else if ( (*t) -> value() == "max" ){

					if (upcast) result.setDouble( std::max(op1_n.doubleCast(), op2_n.doubleCast()) );
					else result.setInt( std::max(op1_n.getInt(), op2_n.getInt()) );
				}
				else throw SyntaxError(*t, "Unrecognised operator for arithmetic evaluation.");

				evalStack.push( new NumericalOperand(result) );
				delete operand1; delete operand2;
			}

		}
		else if ( isOperand(*t) ){

			//check types
			if ((*t) -> identify() != "DoubleLiteral" and (*t) -> identify() != "IntLiteral" and (*t) -> identify() != "Variable"){

				throw WrongType(*t, "Operands must be doubles, ints, or variables.");
			}			

			Numerical result = substituteVariable( *t, param2value, globalVariables, localVariables );
			evalStack.push( new NumericalOperand(result) );
		}
	}

	if ( evalStack.top() -> identify() != "Numerical" ) throw SyntaxError( inputRPN[0], "Expression must evaluate to a numerical value." );

	NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(evalStack.top());
	Numerical result = numptr -> getValue();
	delete evalStack.top();

#if DEBUG_RPN
	if (result.isInt()) std::cout << "Int is: " << result.getInt() << std::endl;
	else std::cout << "Double is: " << result.getDouble() << std::endl;
#endif

	return result;
}


bool evalRPN_condition( std::vector< Token * > inputRPN, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

	std::stack<RPNoperand *> evalStack;	

	for ( auto t = inputRPN.begin(); t < inputRPN.end(); t++ ){

		if ( isOperator(*t) or (*t) -> identify() == "Function"){

			if ( (*t) -> value() == "abs" or (*t) -> value() == "sqrt" or (*t) ->value() == "neg" ){ //unary

				if ( evalStack.size() < 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand = evalStack.top();
				evalStack.pop();
				if (operand -> identify() != "Numerical") throw WrongType(*t,operand -> identify());
				NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand);
				Numerical op_n = numptr -> getValue();
				Numerical result;
				if ( (*t) -> value() == "abs" ){

					if (op_n.isInt()) result.setInt(std::abs(op_n.getInt()));
					else result.setDouble(std::abs(op_n.getDouble()));
				}
				else if ( (*t) -> value() == "sqrt" ){

					if (op_n.isInt()) result.setInt(sqrt(op_n.getInt()));
					else result.setDouble(sqrt(op_n.getDouble()));
				}
				else if ( (*t) -> value() == "neg" ){

					if (op_n.isInt()) result.setInt(-op_n.getInt());
					else result.setDouble(-op_n.getDouble());
				}
				else throw SyntaxError(*t, "Unrecognised operator for function evaluation.");

				evalStack.push( new NumericalOperand(result) );
				delete operand;
			}
			else if ( (*t) -> value() == "min" or (*t) -> value() == "max" or (*t) -> value() == "+" or (*t) -> value() == "-" or (*t) -> value() == "*" or (*t) -> value() == "/" or (*t) -> value() == "^"){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Numerical") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Numerical") throw WrongType(*t,operand2 -> identify());
				Numerical op1_n,op2_n;
				NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand1);
				op1_n = numptr -> getValue();
				numptr = dynamic_cast<NumericalOperand *>(operand2);
				op2_n = numptr -> getValue();

				//upcast
				bool upcast = false;
				if (op1_n.isDouble() or op2_n.isDouble()) upcast = true;

				Numerical result;
				if ( (*t) -> value() == "+" ){

					if (upcast) result.setDouble( op1_n.doubleCast() + op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() + op2_n.getInt() );
				}
				else if ( (*t) -> value() == "-" ){

					if (upcast) result.setDouble( op1_n.doubleCast() - op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() - op2_n.getInt() );
				}
				else if ( (*t) -> value() == "/" ){

					if (upcast) result.setDouble( op1_n.doubleCast() / op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() / op2_n.getInt() );
				}
				else if ( (*t) -> value() == "*" ){
					if (upcast) result.setDouble( op1_n.doubleCast() * op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() * op2_n.getInt() );
				}
				else if ( (*t) -> value() == "^" ){

					if (upcast) result.setDouble( pow(op1_n.doubleCast(), op2_n.doubleCast()) );
					else result.setInt( pow(op1_n.getInt(), op2_n.getInt()) );
				}
				else if ( (*t) -> value() == "min" ){

					if (upcast) result.setDouble( std::min(op1_n.doubleCast(), op2_n.doubleCast()) );
					else result.setInt( std::min(op1_n.getInt(), op2_n.getInt()) );
				}
				else if ( (*t) -> value() == "max" ){

					if (upcast) result.setDouble( std::max(op1_n.doubleCast(), op2_n.doubleCast()) );
					else result.setInt( std::max(op1_n.getInt(), op2_n.getInt()) );
				}
				else throw SyntaxError(*t, "Unrecognised operator for arithmetic evaluation.");

				evalStack.push( new NumericalOperand(result) );
				delete operand1; delete operand2;
			}
			else if ( (*t) -> value() == "==" or (*t) -> value() == "!=" or (*t) -> value() == ">" or (*t) -> value() == "<" or (*t) -> value() == ">=" or (*t) -> value() == "<=" ){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Numerical") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Numerical") throw WrongType(*t,operand2 -> identify());
				Numerical op1_n,op2_n;
				NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand1);
				op1_n = numptr -> getValue();
				numptr = dynamic_cast<NumericalOperand *>(operand2);
				op2_n = numptr -> getValue();

				bool result;
				if ( (*t) -> value() == "==" ){
					result = op1_n.doubleCast() == op2_n.doubleCast();
				}
				else if ( (*t) -> value() == "!=" ){
					result = op1_n.doubleCast() != op2_n.doubleCast();
				}
				else if ( (*t) -> value() == ">" ){
					result = op1_n.doubleCast() > op2_n.doubleCast();
				}
				else if ( (*t) -> value() == "<" ){
					result = op1_n.doubleCast() < op2_n.doubleCast();
				}
				else if ( (*t) -> value() == ">=" ){
					result = op1_n.doubleCast() >= op2_n.doubleCast();
				}
				else if ( (*t) -> value() == "<=" ){
					result = op1_n.doubleCast() <= op2_n.doubleCast();
				}
				else throw SyntaxError(*t, "Unrecognised operator for comparison evaluation.");

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
				else throw SyntaxError(*t, "Unrecognised operator for Boolean evaluation.");

				evalStack.push( new BoolOperand(result) );
				delete operand1; delete operand2;
			}
			
		}
		else if ( isOperand(*t) ){

			//check types
			if ((*t) -> identify() != "DoubleLiteral" and (*t) -> identify() != "IntLiteral" and (*t) -> identify() != "Variable"){

				throw WrongType(*t, "Operands must be doubles, ints, or variables.");
			}			

			Numerical result = substituteVariable( *t, param2value, globalVariables, localVariables );
			evalStack.push( new NumericalOperand(result) );
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


bool evalRPN_set( int &toTest, std::vector< Token * > &inputRPN, ParameterValues &param2value, GlobalVariables &globalVariables, std::map< std::string, Numerical > &localVariables){

#if DEBUG_SETS
std::cout << "Testing: " << toTest << std::endl;
std::cout << "Expression is: ";
for (auto test = inputRPN.begin(); test < inputRPN.end(); test++) std::cout << (*test) -> value();
std::cout << std::endl;
//std::cout << "Parameters are:" << std::endl;
//param2value.printValues();
#endif

	std::stack<RPNoperand *> evalStack;

	for ( auto t = inputRPN.begin(); t < inputRPN.end(); t++ ){

		if ( isOperator(*t) or (*t) -> identify() == "Function"){

			if ( (*t) -> value() == "abs" or (*t) -> value() == "sqrt" or (*t) ->value() == "neg" ){ //unary

				if ( evalStack.size() < 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand = evalStack.top();
				evalStack.pop();
				if (operand -> identify() != "Numerical") throw WrongType(*t,operand -> identify());
				NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand);
				Numerical op_n = numptr -> getValue();
				if (op_n.isDouble()) throw WrongType(*t, "Parameter expressions in message receive must evaluate to ints, not doubles (either through explicit or implicit casting).");

				Numerical result;
				if ( (*t) -> value() == "abs" ){

					if (op_n.isInt()) result.setInt(std::abs(op_n.getInt()));
					else result.setDouble(std::abs(op_n.getDouble()));
				}
				else if ( (*t) -> value() == "sqrt" ){

					if (op_n.isInt()) result.setInt(sqrt(op_n.getInt()));
					else result.setDouble(sqrt(op_n.getDouble()));
				}
				else if ( (*t) -> value() == "neg" ){

					if (op_n.isInt()) result.setInt(-op_n.getInt());
					else result.setDouble(-op_n.getDouble());
				}
				else throw SyntaxError(*t, "Unrecognised operator for function evaluation.");

				evalStack.push( new NumericalOperand(result) );
				delete operand;
			}
			else if ( (*t) -> value() == "min" or (*t) -> value() == "max" or (*t) -> value() == "+" or (*t) -> value() == "-" or (*t) -> value() == "*" or (*t) -> value() == "/" or (*t) -> value() == "^"){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Numerical") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Numerical") throw WrongType(*t,operand2 -> identify());
				Numerical op1_n,op2_n;
				NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand1);
				op1_n = numptr -> getValue();
				numptr = dynamic_cast<NumericalOperand *>(operand2);
				op2_n = numptr -> getValue();
				if (op1_n.isDouble() or op2_n.isDouble()) throw WrongType(*t, "Parameter expressions in message receive must evaluate to ints, not doubles (either through explicit or implicit casting).");

				//upcast
				bool upcast = false;
				if (op1_n.isDouble() or op2_n.isDouble()) upcast = true;

				Numerical result;
				if ( (*t) -> value() == "+" ){

					if (upcast) result.setDouble( op1_n.doubleCast() + op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() + op2_n.getInt() );
				}
				else if ( (*t) -> value() == "-" ){

					if (upcast) result.setDouble( op1_n.doubleCast() - op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() - op2_n.getInt() );
				}
				else if ( (*t) -> value() == "/" ){

					if (upcast) result.setDouble( op1_n.doubleCast() / op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() / op2_n.getInt() );
				}
				else if ( (*t) -> value() == "*" ){
					if (upcast) result.setDouble( op1_n.doubleCast() * op2_n.doubleCast() );
					else result.setInt( op1_n.getInt() * op2_n.getInt() );
				}
				else if ( (*t) -> value() == "^" ){

					if (upcast) result.setDouble( pow(op1_n.doubleCast(), op2_n.doubleCast()) );
					else result.setInt( pow(op1_n.getInt(), op2_n.getInt()) );
				}
				else if ( (*t) -> value() == "min" ){

					if (upcast) result.setDouble( std::min(op1_n.doubleCast(), op2_n.doubleCast()) );
					else result.setInt( std::min(op1_n.getInt(), op2_n.getInt()) );
				}
				else if ( (*t) -> value() == "max" ){

					if (upcast) result.setDouble( std::max(op1_n.doubleCast(), op2_n.doubleCast()) );
					else result.setInt( std::max(op1_n.getInt(), op2_n.getInt()) );
				}
				else throw SyntaxError(*t, "Unrecognised operator for arithmetic evaluation.");

				evalStack.push( new NumericalOperand(result) );
				delete operand1; delete operand2;
			}
			else if ( (*t) -> value() == ".." ){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Numerical") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Numerical") throw WrongType(*t,operand2 -> identify());
				Numerical op1_n,op2_n;
				NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand1);
				op1_n = numptr -> getValue();
				numptr = dynamic_cast<NumericalOperand *>(operand2);
				op2_n = numptr -> getValue();
				if (op1_n.isDouble() or op2_n.isDouble()) throw WrongType(*t, "Parameter expressions in message receive must evaluate to ints, not doubles (either through explicit or implicit casting).");

				bool result;
				if (op1_n.getInt() > op2_n.getInt() ) throw SyntaxError(*t,"Thrown by expression evaluation (sets).  Range upper bound is greater than range lower bound.");
				if (op1_n.getInt() <= toTest and toTest <= op2_n.getInt()) result = true;
				else result = false;

				evalStack.push( new BoolOperand(result) );
				delete operand1; delete operand2;
			}
			else if ( (*t) -> value() == "U" or (*t) -> value() == "I" or (*t) -> value() == "\\" ){

				//get the operands and make sure they're of correct type for the operator
				if ( evalStack.size() <= 1 ) throw SyntaxError(*t, "Insufficient arguments.");
				RPNoperand *operand2 = evalStack.top();
				evalStack.pop();
				RPNoperand *operand1 = evalStack.top();
				evalStack.pop();
				if (operand1 -> identify() != "Numerical" and operand1 -> identify() != "Bool") throw WrongType(*t,operand1 -> identify());
				if (operand2 -> identify() != "Numerical" and operand2 -> identify() != "Bool") throw WrongType(*t,operand2 -> identify());
				Numerical op1_n,op2_n;
				NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand1);
				op1_n = numptr -> getValue();
				numptr = dynamic_cast<NumericalOperand *>(operand2);
				op2_n = numptr -> getValue();
				
				//get everything in bool format
				bool op1_s;
				if (operand1 -> identify() == "Numerical"){

					NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand1);
					Numerical op1_n = numptr -> getValue();
					if (op1_n.isDouble()) throw WrongType(*t, "Parameter expressions in message receive must evaluate to ints, not doubles (either through explicit or implicit casting).");
					op1_s = op1_n.getInt() == toTest;
				}
				else{

					BoolOperand *boolptr = dynamic_cast<BoolOperand *>(operand1);
					op1_s = boolptr -> getValue();
				}

				bool op2_s;
				if (operand2 -> identify() == "Numerical"){

					NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(operand2);
					Numerical op2_n = numptr -> getValue();
					if (op2_n.isDouble()) throw WrongType(*t, "Parameter expressions in message receive must evaluate to ints, not doubles (either through explicit or implicit casting).");
					op2_s = op2_n.getInt() == toTest;
				}
				else{

					BoolOperand *boolptr = dynamic_cast<BoolOperand *>(operand2);
					op2_s = boolptr -> getValue();
				}

				bool result;
				if ( (*t) -> value() == "U" ){
					result = op1_s or op2_s;
				}
				else if ( (*t) -> value() == "I" ){
					result = op1_s and op2_s;
				}
				else if ( (*t) -> value() == "\\" ){
					result = op1_s and not op2_s;
				}
				else throw SyntaxError(*t, "Unrecognised operator for set evaluation.");

				evalStack.push( new BoolOperand(result) );
				delete operand1; delete operand2;
			}
		}
		else if ( isOperand(*t) ){

			//check types
			if ((*t) -> identify() != "DoubleLiteral" and (*t) -> identify() != "IntLiteral" and (*t) -> identify() != "Variable"){

				throw WrongType(*t, "Operands must be doubles, ints, or variables.");
			}			

			Numerical result = substituteVariable( *t, param2value, globalVariables, localVariables );
			evalStack.push( new NumericalOperand(result) );
		}
	}

	if ( evalStack.top() -> identify() != "Bool" and evalStack.top() -> identify() != "Numerical" ) throw SyntaxError( inputRPN[0], "Message receive expression must evaluate to a bool or an int" );

	bool result;
	if ( evalStack.top() -> identify() == "Numerical" ){

		NumericalOperand *numptr = dynamic_cast<NumericalOperand *>(evalStack.top());
		Numerical op1_n = numptr -> getValue();
		if (op1_n.isDouble()) throw WrongType(inputRPN[0], "Parameter expressions in message receive must evaluate to ints, not doubles (either through explicit or implicit casting).");
		result = op1_n.doubleCast() == toTest;
	}
	else{

		BoolOperand *boolptr = dynamic_cast<BoolOperand *>(evalStack.top());
		result = boolptr -> getValue();
	}
	delete evalStack.top();

#if DEBUG_SETS
	std::cout << "Bool is: " << result << std::endl;
#endif
	return result;
}
