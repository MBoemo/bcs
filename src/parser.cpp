//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG_PARSER_PROCCESSDEFS 1
//#define DEBUG_PARSER_VARDEFS 1
//#define DEBUG_GLOBALVAR 1

#include <vector>
#include <algorithm> /*find */
#include <utility>
#include <assert.h>
#include <iostream>
#include <iterator> /*next */
#include <stack>
#include "parser.h"
#include "error_handling.h"


void printTree( Tree<Token> pt, Token *t ){
/*for testing/debugging - prints out the parse tree */

	/*if this token has children */
	if ( not pt.isLeaf( t ) ){

		std::vector< Token * > children = pt.getChildren( t );

		/*print them */
		std::cout << "Parent: " << t -> value() << std::endl; 
		std::cout << "\tChildren: ";
	
		for ( auto child = children.begin(); child < children.end(); child++ ){

			std::cout << (*child) -> value() << " ";
		}
		std::cout << std::endl;

		/*recurse on subtrees */
		for ( auto child = children.begin(); child < children.end(); child++ ){

			printTree( pt, *child );
		}
	}

	if ( pt.isLeaf(t) and pt.isRoot(t) ) std::cout << "Only token: " << t -> value() << std::endl;
}


void checkProcessGrammar( Tree<Token> pt, Token *t ){
/*checks process definition grammar against the BNF */

	/*unary and binary tokens */
	std::vector< std::string > ut = {"BeaconCheck", "BeaconKill", "MessageSend", "MessageReceive", "Action", "Process", "Gate"};
	std::vector< std::string > bt = {"+", "||"};

	if ( pt.isLeaf( t ) ){

		//leaves must be messages, actions, or processes 
		if ( std::find( ut.begin(), ut.end(), t -> identify() ) == ut.end() ) throw SyntaxError( t, "Thrown by parser: Leaf nodes must be messages, actions, or processes." );
		if ( t -> identify() == "Gate" ) throw SyntaxError( t, "Thrown by parser: This gate does not guard an action." );
		return;
	}

	std::vector< Token * > children = pt.getChildren( t );

	if ( t -> value() == "=" ){

		/*must be binary */
		if ( children.size() != 2 ) throw SyntaxError( t, "Thrown by parser: Assignment must have two arguments." );

		/*LHS must be process, RHS must be +, ||, or action */
		if ( children[0] -> identify() != "Process" ) throw SyntaxError( children[0], "Thrown by parser: Left hand side of this assignment must be a process." );
		if ( std::find( ut.begin(), ut.end(), children[1] -> identify() ) == ut.end() and std::find( bt.begin(), bt.end(), children[1] -> value() ) == bt.end() ){

			throw SyntaxError( children[1], "Thrown by parser: Right hand side of this assignment must be an action, unary, or binary operator." ); 
		}

		/*recurse down the RHS */
		checkProcessGrammar( pt, children[1] );
	}
	else if ( t -> value() == "+" or t -> value() == "||" ){
		
		/*must be binary */
		if ( children.size() != 2 ) throw SyntaxError( t, "Thrown by parser: Choice/parallel must have two arguments." );

		/*must have the right children */
		if ( std::find( ut.begin(), ut.end(), children[0] -> identify() ) == ut.end() and std::find( bt.begin(), bt.end(), children[0] -> value() ) == bt.end() ){

			throw SyntaxError( children[0], "Thrown by parser: Arguments to choice/parallel must be processes, actions, unary, or binary operators." ); 
		}
		if ( std::find( ut.begin(), ut.end(), children[1] -> identify() ) == ut.end() and std::find( bt.begin(), bt.end(), children[1] -> value() ) == bt.end() ){

			throw SyntaxError( children[1], "Thrown by parser: Arguments to choice/parallel must be processes, actions, unary, or binary operators." ); 
		}

		/*recurse down */
		checkProcessGrammar( pt, children[0] );
		checkProcessGrammar( pt, children[1] );
	}
	else if ( t -> identify() == "Process" ){
	
		//processes must be leaves
		if ( not pt.isLeaf( t ) ) throw SyntaxError( t, "Thrown by parser: A process can not be used as prefix." );
	}
	else if ( std::find( ut.begin(), ut.end(), t -> identify() ) != ut.end() ){

		/*must be unary */
		if ( children.size() != 1 ) throw SyntaxError( t, "Thrown by parser: Unary operators can only have one operand." );

		/*must have the right children */
		if ( std::find( ut.begin(), ut.end(), children[0] -> identify() ) == ut.end() and std::find( bt.begin(), bt.end(), children[0] -> value() ) == bt.end() ){

			throw SyntaxError( children[0], "Thrown by parser: Unary operators must have a process, action, unary, or binary operator as an operand." ); 
		}

		/*recurse down */
		checkProcessGrammar( pt, children[0] );
	}
	else throw SyntaxError( t, "Thrown by parser: Unrecognised token value in process grammar check." );
}


void matchParentheses( std::vector< Token * > tokenisedLine ){
/*checks a tokenised line to make sure all parentheses are balanced, throws an error if not */

	std::stack< Token * > parenStack;

	for ( auto i = tokenisedLine.begin(); i < tokenisedLine.end(); i++ ){

		if ( ( (*i) -> identify() == "Parentheses" ) and ( (*i) -> value() == "(" ) ) {

			parenStack.push( *i );
		}
		else if ( ( (*i) -> identify() == "Parentheses" ) and ( (*i) -> value() == ")" ) ) {

			if ( parenStack.empty() ) throw UnbalancedParentheses( *i );

			parenStack.pop();
		}
	}
	if ( not parenStack.empty() ) throw UnbalancedParentheses( parenStack.top() );
}


std::vector< Token * > parseSystemLine( std::vector< Token * > &tokenisedLine ){
/*checks grammar on tokenised system line and strips out parallel operators */

	std::vector< Token * > tokenisedSystemLine;
	int flip = 0;

	/*check the end to make sure we trail with a process */
	if ( tokenisedLine.back() -> identify() != "Process" ){

		throw SyntaxError ( tokenisedLine.back(), "Thrown by parser: System line must trail with a process or local variable assignment." );
	}

	/*make sure process and parallel operators alternate */
	for ( auto t = tokenisedLine.begin(); t < tokenisedLine.end(); t++ ){

		if ( (*t) -> identify() == "Process" and flip == 0 ){

			flip++; flip %= 2;
			tokenisedSystemLine.push_back( *t );
			continue;
		} 
		else if ( ((*t) -> identify() == "Variable" or (*t) -> identify() == "IntLiteral") and (*(t+1)) -> value() == "*" and (*(t+2)) -> identify() == "Process" ){

			tokenisedSystemLine.push_back( *t );
			continue;
		}
		else if ( (*t) -> value() == "*" and ( (*(t-1)) -> identify() == "Variable" or (*(t-1)) -> identify() == "IntLiteral") and (*(t+1)) -> identify() == "Process" ){

			continue;
		}
		else if ( (*t) -> value() == "||" and flip == 1 ){

			flip++; flip %= 2;
			continue;
		}
		else throw SyntaxError( *t, "Thrown by parser: Illegal token type in system line or system line formatted incorrectly." );
	}
	return tokenisedSystemLine;
}


void parseDefLine( Token *parentToken, std::vector< Token * > &tokenisedLine, Tree<Token> &treeForLine ){
/*called by parseSource, creates a parse tree for a single line of the source code */

	/*operators */
	std::vector< std::string > bindingOrder = {"+", "||"};

	/*vector for tokens for right hand side and left hand side for the pivot */
	std::vector< Token * > LHS, RHS;

	/*stack for parentheses matching */
	std::stack< Token * > parenStack;

	for ( auto binaryOperator = bindingOrder.begin(); binaryOperator < bindingOrder.end(); binaryOperator++ ){

		/*scan until you find the leftmost operator that we're looking for */
		for ( auto t = tokenisedLine.begin(); t < tokenisedLine.end(); t++ ){

			/*pop matching parentheses on/off the stack */
			if ( (*t) -> value() == "(" ) parenStack.push( *t );
			else if ( (*t) -> value() == ")" ) parenStack.pop();

			/*find pivot */
			if ( (*t) -> value() == *binaryOperator and parenStack.empty() ){

				/*push the operator to the tree */				
				treeForLine.addChild( parentToken, *t );

				/*separate into LHS and RHS of pivot */
				RHS.insert( RHS.end(), std::next(t), tokenisedLine.end() );
				LHS.insert( LHS.end(), tokenisedLine.begin(), t );

				/*recurse */
				parseDefLine( *t, LHS, treeForLine );
				parseDefLine( *t, RHS, treeForLine );
				return;
			}
		}
	}

	/*when we're done with the binary operators, we need to handle the unary ones (prefix and gates) */
	for ( auto t = tokenisedLine.begin(); t < tokenisedLine.end(); t++ ){

		/*pop matching parentheses on/off the stack */
		if ( (*t) -> value() == "(" ) parenStack.push( *t );
		else if ( (*t) -> value() == ")" ) parenStack.pop();

		/*find pivot using reverse iterators */
		if ( ( (*t) -> value() == "." or (*t) -> identify() == "Gate" ) and parenStack.empty() ){

			/*separate into LHS and RHS of pivot */
			RHS.insert( RHS.end(), std::next(t), tokenisedLine.end() );
			LHS.insert( LHS.end(), tokenisedLine.begin(), t );

			if ( (*t) -> identify() == "Gate" ){

				/*at this point, the tokenised line should look something like [g] -> B.C so make sure LHS is empty*/
				if ( LHS.size() != 0 ) throw SyntaxError( LHS[0], "Thrown by parser: Could not parse gate - check syntax." );

				/*push to the tree */
				treeForLine.addChild( parentToken, *t );

				/*recurse on the LHS (the RHS was added already) */
				parseDefLine( *t, RHS, treeForLine );
				return;
			}
			else if ( (*t) -> value() == "." ){

				/*at this point, the tokenised line should look something like A.B.C so make sure LHS just has an action*/
				if ( LHS.size() != 1 ) throw SyntaxError( LHS[0], "Thrown by parser: Could not parse prefix action - check syntax." );

				/*push to the tree */
				treeForLine.addChild( parentToken, LHS[0] );

				/*recurse on the LHS (the RHS was added already) */
				parseDefLine( LHS[0], RHS, treeForLine );
				return;
			}
		}
	}
	
	/*there are no combinators left.  At this point, the tokenised line should be a single token (leaf node) unless there are parentheses left to resolve */
	if ( tokenisedLine.size() > 1 ){

		/*if this line segment is enclosed by parentheses, erase those parentheses */
		if ( (*(tokenisedLine.begin())) -> value() == "(" and (*(tokenisedLine.end() - 1)) -> value() == ")"  ) {

			tokenisedLine.erase( tokenisedLine.begin() );
			tokenisedLine.erase( tokenisedLine.end() - 1 );
		}

		parseDefLine( parentToken, tokenisedLine, treeForLine );
		return;
	}

	treeForLine.addChild( parentToken, tokenisedLine[0] );
}


std::tuple< std::vector< Tree<Token> >, std::vector<Token *>, GlobalVariables > parseSource( std::vector< std::vector< Token * > > &tokenisedSource ){
/*creates a vector of parse trees, one for each line in the source code.  this is the main parsing function */

	std::vector< Tree<Token> > treesFromSource;
	std::vector< Token * > tokenisedSystemLine;
	GlobalVariables variableName2Value;

	bool systemLineFound = false;

	/*for each tokenised line */
	for ( auto tokenisedLine = tokenisedSource.begin(); tokenisedLine < tokenisedSource.end(); tokenisedLine++ ){

		/*vector for tokens for right hand side and left hand side for the pivot */
		std::vector< Token * > LHS, RHS;

		/*check for balanced parentheses */
		matchParentheses( *tokenisedLine );

		bool definitionLine = false;

		/*for each token in that line, find the first parent token (which will be the root of the tree).  then start recursion */
		for ( auto token = tokenisedLine -> begin(); token < tokenisedLine -> end(); token++ ){

			/*find pivot */
			if ( (*token) -> value() == "=" ){

				/*root the parse tree with the pivot */
				Tree<Token> treeForLine;
				treeForLine.setRoot( *token );
				
				/*separate into LHS and RHS of pivot */
				RHS.insert( RHS.end(), std::next(token), tokenisedLine -> end() );
				LHS.insert( LHS.end(), tokenisedLine -> begin(), token );

				if ( LHS.size() != 1 ) throw SyntaxError( *token, "Thrown by parser: Left hand side of assignment must be a single process name or variable name." );
				if ( RHS.size() == 0 ) throw SyntaxError( *token, "Thrown by parser: Right hand side of assignment cannot be empty." );

				/*if the LHS is a process, parse this definition as a process definition */
				if ( LHS[0] -> identify() == "Process" ){

					/*recurse */
					parseDefLine( *token, LHS, treeForLine );
					parseDefLine( *token, RHS, treeForLine );

					checkProcessGrammar( treeForLine, treeForLine.getRoot() );
					treesFromSource.push_back( treeForLine );

#if DEBUG_PARSER_PROCCESSDEFS
printTree( treeForLine, treeForLine.getRoot() );
#endif
				}
				/*if the LHS is a variable, parse this definition as a variable definition */
				else if ( LHS[0] -> identify() == "Variable" ){

					/*we need an int literal or double literal on the RHS of the equal sign */
					if ( RHS.size() != 1 ) throw SyntaxError( *token, "Thrown by parser: Right hand side of variable assignment must be an int literal or double literal." );
					if ( RHS[0] -> identify() == "DoubleLiteral"){ 

						variableName2Value.updateValue(LHS[0] -> value(), std::stod(RHS[0] -> value()));
					}
					else if ( RHS[0] -> identify() == "IntLiteral"){

						variableName2Value.updateValue(LHS[0] -> value(), std::stoi(RHS[0] -> value()));
					}
					else throw SyntaxError( RHS[0], "Thrown by parser: Right hand side of variable assignment must be an int literal or double literal." );

#if DEBUG_PARSER_VARDEFS
std::cout << RHS[0] -> value() << " assigned to " << LHS[0] -> value() << std::endl;
#endif		
				}
				else{
		
					throw SyntaxError( LHS[0], "Thrown by parser: Left hand side of assignment must be a variable or process name" );
				}
				definitionLine = true;
				break;
			}
		}

		if ( not definitionLine ){
			if ( systemLineFound ) throw MultipleSystemLines();
			systemLineFound = true;
			tokenisedSystemLine = parseSystemLine( *tokenisedLine );
		}
	}

#if DEBUG_GLOBALVAR
variableName2Value.printValues();
#endif

	if ( not systemLineFound ) throw SystemLineMissing();

#if defined DEBUG_PARSER_PROCCESSDEFS || defined DEBUG_PARSER_VARDEFS
exit(EXIT_SUCCESS);
#endif
	return make_tuple( treesFromSource, tokenisedSystemLine, variableName2Value );
}
