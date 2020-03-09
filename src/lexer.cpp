//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <assert.h>
#include "lexer.h"
#include "error_handling.h"
#include "parser.h"


/*AUTOMATON METHODS--------------------------------------------------------------------------------------------------------------------------------------------------*/
void FiniteStateAutomaton::add_edge( std::string inState, std::set< char > acceptingChars, std::string outState ){
//adds an edge to a finite state automaton
//args:
// - inState: string of the state name that we're coming from
// - acceptingChars: if we see any of these characters, we transition from inState to outState in the automaton
// - outState: string of the state name that we're going to

	/*safety check: take the edge you want to add, check all existing edges, and make sure you're not making the automaton stochastic */
	for ( auto edg = edges.begin(); edg < edges.end(); edg++ ){
	
		std::string existingFromState = std::get<0>(*edg);
		std::set< char > edgeChars = std::get<1>(*edg);
		std::set< char > intersection;
	
		std::set_intersection( edgeChars.begin(), edgeChars.end(), acceptingChars.begin(), acceptingChars.end(), std::inserter( intersection,intersection.begin() ) );

		//catch nondeterministic automata
		if ( inState == existingFromState ) assert( intersection.size() == 0 );
	}
	edges.push_back( make_tuple( inState, acceptingChars, outState ) );
}


void FiniteStateAutomaton::designate_endState( std::string newEndState ){
//designates newEndState as an end state for the machine

	endStates.push_back( newEndState );
}


std::string FiniteStateAutomaton::testString( std::string strToTest ){
//takes strToTest and tests if it's valid using the automaton
//returns:
// - empty string if rejected, which is to say we can't transition from a start state to an end state in this automaton
// - the accepted string, a portion of strToTest

	std::string currentState = startState;
	std::string acceptedString;

	/*require that we have at least one end state */
	assert( endStates.size() > 0 );

	/*iterate on symbols in the test string */
	for ( unsigned int i = 0; i < strToTest.size(); i++ ){

		bool foundEdge = false;

		/*iterate on possible edges we can take from this state */
		for ( auto edg = edges.begin(); edg < edges.end(); edg++ ){

			std::string fromState = std::get<0>(*edg);

			/*if the edge leads from the current state... */
			if ( fromState == currentState ){

				std::set< char > acceptingSymbols = std::get<1>(*edg);

				/*if this edge accepts the symbol */
				if ( acceptingSymbols.count( strToTest[i] ) ){

					/*switch the current state to the state that the edge leads to, append the symbol to the accepted string */
					currentState = std::get<2>(*edg);
					acceptedString += strToTest[i];
					foundEdge=true;
					break;
				}
			}
		}

		/*termination conditions - if I have a symbol where there's no viable edge from the state I'm in, OR i've reached the end of the test string */
		if ( (not foundEdge) or (i == strToTest.size() - 1) ){

			/*if we're in an end state, then we're okay - terminate the machine successfully and return the accepted string */
			if ( std::find( endStates.begin(), endStates.end(), currentState ) != endStates.end() ){

				break;
			}
			/*if we're not in the end state, then don't accept anything and return the empty string */
			else {

				acceptedString = "";
				break;
			}
		}
	}
	return acceptedString;	
}


/*VARS FOR LEXING----------------------------------------------------------------------------------------------------------------------------------------------------*/
FiniteStateAutomaton BeaconCheckTestMachine, 
		     BeaconKillTestMachine, 
		     MessageSendTestMachine, 
		     MessageReceiveTestMachine, 
		     ActionTestMachine,
		     ProcessTestMachine, 
		     VariableTestMachine,
		     DoubleTestMachine,
		     IntTestMachine,
		     WhitespaceTestMachine,
		     GateTestMachine,
		     ComparisonTestMachine,
		     OperatorTestMachine,
		     AssignmentTestMachine,
		     ParenthesesTestMachine,
		     CommaTestMachine,
		     MessagePrimitiveTestMachine,
		     ParameterTestMachine,
		     SetOperatorTestMachine,
		     SemicolonTestMachine;

/*useful sets for lexicographical analysis */
std::set< char > setAlpha = {'A','B','C','D','E','F','G','H','I','J','K','L',
			     'M','N','O','P','Q','R','S','T','U','V','W','X',
			     'Y','Z','a','b','c','d','e','f','g','h','i','j',
			     'k','l','m','n','o','p','q','r','s','t','u','v',
			     'w','x','y','z'};

std::set< char > setNumeric = {'0','1','2','3','4','5','6','7','8','9'};

std::vector< Token * > scanLine( std::string &line, unsigned int lineNumber, unsigned int colNumber ){
//scans a line (as a string) and lexes that line into tokens, returns the ordered tokens as a vector

	std::vector< std::pair< FiniteStateAutomaton, std::string > > machTokenPairs= { std::make_pair( BeaconCheckTestMachine, "BeaconCheck" ),
											std::make_pair( BeaconKillTestMachine, "BeaconKill" ),
											std::make_pair( MessageSendTestMachine, "MessageSend" ),
											std::make_pair( MessageReceiveTestMachine, "MessageReceive" ),
											std::make_pair( ActionTestMachine, "Action" ),
											std::make_pair( ProcessTestMachine, "Process" ),
											std::make_pair( SetOperatorTestMachine, "SetOperation" ),
											std::make_pair( VariableTestMachine, "Variable" ),
											std::make_pair( DoubleTestMachine, "DoubleLiteral" ),				  
											std::make_pair( IntTestMachine, "IntLiteral" ),
											std::make_pair( WhitespaceTestMachine, "Whitespace" ),
											std::make_pair( GateTestMachine, "Gate" ),
											std::make_pair( ParameterTestMachine, "ParameterCondition" ),
											std::make_pair( OperatorTestMachine, "Operator" ),
											std::make_pair( ComparisonTestMachine, "Comparison" ),
											std::make_pair( AssignmentTestMachine, "Assignment" ),
											std::make_pair( ParenthesesTestMachine, "Parentheses" ),
											std::make_pair( CommaTestMachine, "Comma" ),
											std::make_pair( MessagePrimitiveTestMachine, "MessagePrimitive" ),
											std::make_pair( SemicolonTestMachine, "Semicolon" ) };

	std::vector< Token * > tokenisedLine;
	std::string testOutcome;

	while ( not line.empty() ){

		bool tokenFound = false;

		for ( auto machine = machTokenPairs.begin(); machine < machTokenPairs.end(); machine++ ){

			std::string testOutcome = ((*machine).first).testString( line );
			if ( testOutcome != "" ){

				/*ignore whitespace */
				if ( (*machine).second == "Whitespace" ){

					line = line.substr( testOutcome.size() );
					colNumber += testOutcome.size();
					tokenFound = true;
				}
				else if ( (*machine).second == "DoubleLiteral" and not line.substr( testOutcome.size() ).empty() ){

					if (line.substr(testOutcome.size(), 1) == "."){

						goto isARange;
					}
					else{

						tokenisedLine.push_back( new Token( (*machine).second, testOutcome, lineNumber, colNumber ) );
						line = line.substr( testOutcome.size() );
						colNumber += testOutcome.size();
						tokenFound = true;
					}
				}
				else if ( (*machine).second == "Variable" and (testOutcome == "abs" or testOutcome == "min" or testOutcome == "max" or testOutcome == "sqrt") ){

					tokenisedLine.push_back( new Token( "Function", testOutcome, lineNumber, colNumber ) );
					line = line.substr( testOutcome.size() );
					colNumber += testOutcome.size();
					tokenFound = true;
				}
				else{

					tokenisedLine.push_back( new Token( (*machine).second, testOutcome, lineNumber, colNumber ) );
					line = line.substr( testOutcome.size() );
					colNumber += testOutcome.size();
					tokenFound = true;
				}
				break;
			}
			isARange: ;
		}
		if ( not tokenFound ) throw NoMachinePath( line, lineNumber, colNumber );
	}
	return tokenisedLine;
};


std::vector< std::vector< Token * > > scanSource( std::string &sourceFilename ){
//main lexer function, calls scanLine on each line, contains definitions for automata to do the tokenisation
//arguments:
// - sourceFilename: a string that's the path to the source code

	/*MACHINES */

	/*accepts message primitives (~,_,?,!,#) */
	MessagePrimitiveTestMachine.designate_endState( "endState" );
	MessagePrimitiveTestMachine.add_edge( MessagePrimitiveTestMachine.startState, {'~','@','?','!','#'}, "endState" );

	/*accepts what's between square brackets on messages */
	ParameterTestMachine.designate_endState( "endState" );
	ParameterTestMachine.add_edge( MessagePrimitiveTestMachine.startState, {'['}, "q1" );
	ParameterTestMachine.add_edge( "q1", setAlpha, "q1" );
	ParameterTestMachine.add_edge( "q1", setNumeric, "q1" );
	ParameterTestMachine.add_edge( "q1", {'.'}, "q2" );
	ParameterTestMachine.add_edge( "q2", {'.'}, "q1" );
	ParameterTestMachine.add_edge( "q1", {'\\', '+', '*', '-', '(', ')', ' ','_','|',','}, "q1" );
	ParameterTestMachine.add_edge( "q1", {']'}, "endState" );

	/*accepts operators (+,-,||,.,") */
	OperatorTestMachine.designate_endState( "endState" );
	OperatorTestMachine.add_edge( OperatorTestMachine.startState, {'+','"', '*', '-', '.','/','^'}, "endState" );
	OperatorTestMachine.add_edge( OperatorTestMachine.startState, {'|'}, "q1" );
	OperatorTestMachine.add_edge( "q1", {'|'}, "endState" );

	/*accepts set operators (U, I, \, ..) */
	SetOperatorTestMachine.designate_endState( "endState" );
	SetOperatorTestMachine.add_edge( SetOperatorTestMachine.startState, {'U', 'I', '\\'}, "endState" );
	SetOperatorTestMachine.add_edge( SetOperatorTestMachine.startState, {'.'}, "q1" );
	SetOperatorTestMachine.add_edge( "q1", {'.'}, "endState" );

	/*accepts assignment = */
	AssignmentTestMachine.designate_endState( "endState" );
	AssignmentTestMachine.add_edge( AssignmentTestMachine.startState, {'='}, "endState" );

	/*accepts parentheses */
	ParenthesesTestMachine.designate_endState( "endState" );
	ParenthesesTestMachine.add_edge( ParenthesesTestMachine.startState, {')','('}, "endState" );

	/*accepts a comma */
	CommaTestMachine.designate_endState( "endState" );
	CommaTestMachine.add_edge( CommaTestMachine.startState, {','}, "endState" );

	/*accepts a comma */
	SemicolonTestMachine.designate_endState( "endState" );
	SemicolonTestMachine.add_edge( SemicolonTestMachine.startState, {';'}, "endState" );

	/*accepts a boolean comparison ( <, >, ==, <=, >=, !=, ~ ) */
	ComparisonTestMachine.designate_endState( "es1" );
	ComparisonTestMachine.designate_endState( "es2" );
	ComparisonTestMachine.add_edge( ComparisonTestMachine.startState, {'|', '~', '&'}, "es2" );
	ComparisonTestMachine.add_edge( ComparisonTestMachine.startState, {'>', '<'}, "es1" );
	ComparisonTestMachine.add_edge( "es1", {'='}, "es2" );
	ComparisonTestMachine.add_edge( ComparisonTestMachine.startState, {'=','!'}, "q1" );
	ComparisonTestMachine.add_edge( "q1", {'='}, "es2" );

	/*accepts a variable name */
	VariableTestMachine.designate_endState( "endState" );
	VariableTestMachine.add_edge( VariableTestMachine.startState, setAlpha, "endState" );
	VariableTestMachine.add_edge( VariableTestMachine.startState, {'_'}, "endState" );
	VariableTestMachine.add_edge( "endState", setAlpha, "endState" );
	VariableTestMachine.add_edge( "endState", {'_'}, "endState" );
	VariableTestMachine.add_edge( "endState", setNumeric, "endState" );

	/*accepts an int literal */
	IntTestMachine.designate_endState( "endState" );
	IntTestMachine.add_edge( IntTestMachine.startState, setNumeric, "endState" );
	IntTestMachine.add_edge( "endState", setNumeric, "endState" );

	/*accepts a double literal */
	DoubleTestMachine.designate_endState( "endState" );
	DoubleTestMachine.add_edge( DoubleTestMachine.startState, setNumeric, "q1" );
	DoubleTestMachine.add_edge( DoubleTestMachine.startState, {'.'}, "q2" );
	DoubleTestMachine.add_edge( "q2", setNumeric, "endState" );
	DoubleTestMachine.add_edge( "q1", setNumeric, "q1" );
	DoubleTestMachine.add_edge( "q1", {'.'}, "endState" );
	DoubleTestMachine.add_edge( "endState", setNumeric, "endState" );

	/*accepts a string of whitespace */
	WhitespaceTestMachine.designate_endState( "endState" );
	WhitespaceTestMachine.add_edge( WhitespaceTestMachine.startState, {' ','\f','\n','\r','\t','\v'}, "endState" );
	WhitespaceTestMachine.add_edge( "endState", {' '}, "endState" );

	/*accepts a process definition of the form P[a, b, ..., c] */
	ProcessTestMachine.designate_endState( "endState1" );
	ProcessTestMachine.add_edge( ProcessTestMachine.startState, setAlpha, "q1" );
	ProcessTestMachine.add_edge( "q1", setAlpha, "q1" );
	ProcessTestMachine.add_edge( "q1", setNumeric, "q1" );
	ProcessTestMachine.add_edge( "q1", {'_'}, "q1" );
	ProcessTestMachine.add_edge( "q1", {'['}, "q2" );
	ProcessTestMachine.add_edge( "q2", setAlpha, "q2" );
	ProcessTestMachine.add_edge( "q2", setNumeric, "q2" );
	ProcessTestMachine.add_edge( "q2", {'_',' ',',','+','-','*','.','^','(',')','/'}, "q2" );
	ProcessTestMachine.add_edge( "q2", {']'}, "endState1" );

	/*accepts gates */
	GateTestMachine.designate_endState( "endState" );
	GateTestMachine.add_edge( GateTestMachine.startState, {'['}, "q1" );
	GateTestMachine.add_edge( "q1", {' '}, "q1" );
	GateTestMachine.add_edge( "q1", setNumeric, "q1" );
	GateTestMachine.add_edge( "q1", setAlpha, "q1" );
	GateTestMachine.add_edge( "q1", {'>', '<', '~', '=', '!', '|', '&','(',')','_','.','+','-','*','^','/'}, "q1" );
	GateTestMachine.add_edge( "q1", {']'}, "q8" );
	GateTestMachine.add_edge( "q8", {' '}, "q8" );
	GateTestMachine.add_edge( "q8", {'-'}, "q9" );
	GateTestMachine.add_edge( "q9", {'>'}, "endState" );

	/*ACTION */
	/*accepts an action term of the form {action_name, action_rate} */
	ActionTestMachine.designate_endState( "endState" );
	ActionTestMachine.add_edge( ActionTestMachine.startState, {'{'}, "q1" );
	ActionTestMachine.add_edge( "q1", {' '}, "q1" );
	ActionTestMachine.add_edge( "q1", setAlpha, "q2" );
	ActionTestMachine.add_edge( "q1", {'_'}, "q2" );
	ActionTestMachine.add_edge( "q2", setAlpha, "q2" );
	ActionTestMachine.add_edge( "q2", setNumeric, "q2" );
	ActionTestMachine.add_edge( "q2", {'_'}, "q2" );
	ActionTestMachine.add_edge( "q2", {' '}, "q3" );
	ActionTestMachine.add_edge( "q3", {' '}, "q3" );
	ActionTestMachine.add_edge( "q3", {','}, "q4" );
	ActionTestMachine.add_edge( "q2", {','}, "q4" );
	ActionTestMachine.add_edge( "q4", setAlpha, "q4" );
	ActionTestMachine.add_edge( "q4", setNumeric, "q4" );
	ActionTestMachine.add_edge( "q4", {'_',' ',',','+','-','*','^','(',')','.','/'}, "q4" );
	ActionTestMachine.add_edge( "q4", {'}'}, "endState" );

	/*MESSAGING */
	/*accepts a handshake send or beacon launch {@channelName![i],rate} or {channelName![i],rate} */
	MessageSendTestMachine.designate_endState( "endState" );
	MessageSendTestMachine.add_edge( MessageSendTestMachine.startState, {'{'}, "q1" );
	MessageSendTestMachine.add_edge( "q1", {' '}, "q1" );
	MessageSendTestMachine.add_edge( "q1", {'@'}, "q2" );
	MessageSendTestMachine.add_edge( "p1", {' '}, "s1" );
	MessageSendTestMachine.add_edge( "s1", {' '}, "s1" );

	MessageSendTestMachine.add_edge( "q1", {'_',',','+','/','-','^','*','(',')','.'}, "q2" );
	MessageSendTestMachine.add_edge( "q1", setAlpha, "q2" );
	MessageSendTestMachine.add_edge( "q1", setNumeric, "q2" );

	MessageSendTestMachine.add_edge( "q2", {'_',' ',',','+','/','-','^','*','(',')','.'}, "q2" );
	MessageSendTestMachine.add_edge( "q2", setAlpha, "q2" );
	MessageSendTestMachine.add_edge( "q2", setNumeric, "q2" );

	MessageSendTestMachine.add_edge( "q2", {'!'}, "q3" );
	MessageSendTestMachine.add_edge( "q3", {'['}, "q4" );
	MessageSendTestMachine.add_edge( "q4", {'_',' ',',','+','/','-','^','*','(',')','.'}, "q4" );
	MessageSendTestMachine.add_edge( "q4", setAlpha, "q4" );
	MessageSendTestMachine.add_edge( "q4", setNumeric, "q4" );
	MessageSendTestMachine.add_edge( "q4", {']'}, "q5" );
	MessageSendTestMachine.add_edge( "q5", {' '}, "q6" );
	MessageSendTestMachine.add_edge( "q6", {' '}, "q6" );
	MessageSendTestMachine.add_edge( "q6", {','}, "q7" );
	MessageSendTestMachine.add_edge( "q5", {','}, "q7" );
	MessageSendTestMachine.add_edge( "q7", setAlpha, "q7" );
	MessageSendTestMachine.add_edge( "q7", setNumeric, "q7" );
	MessageSendTestMachine.add_edge( "q7", {'_',' ',',','+','-','*','^','.','(',')','/'}, "q7" );
	MessageSendTestMachine.add_edge( "q7", {'}'}, "endState" );

	/*accepts a handshake receive or beacon receive {@channelName?[i](x),rate} or {channelName?[i],rate} */
	MessageReceiveTestMachine.designate_endState( "endState" );
	MessageReceiveTestMachine.add_edge( MessageReceiveTestMachine.startState, {'{'}, "q1" );
	MessageReceiveTestMachine.add_edge( "q1", {' '}, "q1" );
	MessageReceiveTestMachine.add_edge( "q1", {'@'}, "q2" );
	MessageReceiveTestMachine.add_edge( "p1", {' '}, "s1" );
	MessageReceiveTestMachine.add_edge( "s1", {' '}, "s1" );

	MessageReceiveTestMachine.add_edge( "q1", {'_',',','+','/','-','^','*','(',')','.'}, "q2" );
	MessageReceiveTestMachine.add_edge( "q1", setAlpha, "q2" );
	MessageReceiveTestMachine.add_edge( "q1", setNumeric, "q2" );

	MessageReceiveTestMachine.add_edge( "q2", {'_',' ',',','+','/','-','^','*','(',')','.'}, "q2" );
	MessageReceiveTestMachine.add_edge( "q2", setAlpha, "q2" );
	MessageReceiveTestMachine.add_edge( "q2", setNumeric, "q2" );

	MessageReceiveTestMachine.add_edge( "q2", {'?'}, "q3" );
	MessageReceiveTestMachine.add_edge( "q3", {'['}, "q4" );
	MessageReceiveTestMachine.add_edge( "q4", {'_',' ',',','+','-','/','*','^','(',')','.','\\'}, "q4" );
	MessageReceiveTestMachine.add_edge( "q4", setAlpha, "q4" );
	MessageReceiveTestMachine.add_edge( "q4", setNumeric, "q4" );
	MessageReceiveTestMachine.add_edge( "q4", {']'}, "q5" );
	MessageReceiveTestMachine.add_edge( "q5", {'('}, "r1" );
	MessageReceiveTestMachine.add_edge( "r1", {' '}, "r4" );
	MessageReceiveTestMachine.add_edge( "r4", {' '}, "r4" );
	MessageReceiveTestMachine.add_edge( "r4", setAlpha, "r2" );
	MessageReceiveTestMachine.add_edge( "r4", {'_'}, "r2" );
	MessageReceiveTestMachine.add_edge( "r1", setAlpha, "r2" );
	MessageReceiveTestMachine.add_edge( "r1", {'_'}, "r2" );
	MessageReceiveTestMachine.add_edge( "r2", setAlpha, "r2" );
	MessageReceiveTestMachine.add_edge( "r2", setNumeric, "r2" );
	MessageReceiveTestMachine.add_edge( "r2", {'_'}, "r2" );
	MessageReceiveTestMachine.add_edge( "r2", {' '}, "r3" );
	MessageReceiveTestMachine.add_edge( "r3", {' '}, "r3" );
	MessageReceiveTestMachine.add_edge( "r3", {','}, "r1" );
	MessageReceiveTestMachine.add_edge( "r2", {','}, "r1" );
	MessageReceiveTestMachine.add_edge( "r2", {')'}, "q6" );
	MessageReceiveTestMachine.add_edge( "r3", {')'}, "q6" );
	MessageReceiveTestMachine.add_edge( "q5", {' '}, "q6" );
	MessageReceiveTestMachine.add_edge( "q6", {' '}, "q6" );
	MessageReceiveTestMachine.add_edge( "q6", {','}, "q7" );
	MessageReceiveTestMachine.add_edge( "q5", {','}, "q7" );
	MessageReceiveTestMachine.add_edge( "q7", setAlpha, "q7" );
	MessageReceiveTestMachine.add_edge( "q7", setNumeric, "q7" );
	MessageReceiveTestMachine.add_edge( "q7", {'_',' ',',','+','-','*','^','.','(',')','/'}, "q7" );
	MessageReceiveTestMachine.add_edge( "q7", {'}'}, "endState" );

	/*accepts a beacon check {~channelName?[i], rate} */
	BeaconCheckTestMachine.designate_endState( "endState" );
	BeaconCheckTestMachine.add_edge( BeaconCheckTestMachine.startState, {'{'}, "q1" );
	BeaconCheckTestMachine.add_edge( "q1", {' '}, "q1" );
	BeaconCheckTestMachine.add_edge( "q1", {'~'}, "q2" );
	BeaconCheckTestMachine.add_edge( "p1", {' '}, "s1" );
	BeaconCheckTestMachine.add_edge( "s1", {' '}, "s1" );


	BeaconCheckTestMachine.add_edge( "q1", {'_',',','+','/','-','^','*','(',')','.'}, "q2" );
	BeaconCheckTestMachine.add_edge( "q1", setAlpha, "q2" );
	BeaconCheckTestMachine.add_edge( "q1", setNumeric, "q2" );

	BeaconCheckTestMachine.add_edge( "q2", {'_',' ',',','+','/','-','^','*','(',')','.'}, "q2" );
	BeaconCheckTestMachine.add_edge( "q2", setAlpha, "q2" );
	BeaconCheckTestMachine.add_edge( "q2", setNumeric, "q2" );

	BeaconCheckTestMachine.add_edge( "q2", {'?'}, "q3" );
	BeaconCheckTestMachine.add_edge( "q3", {'['}, "q4" );
	BeaconCheckTestMachine.add_edge( "q4", {'_',' ',',','+','-','/','^','*','(',')','.','\\'}, "q4" );
	BeaconCheckTestMachine.add_edge( "q4", setAlpha, "q4" );
	BeaconCheckTestMachine.add_edge( "q4", setNumeric, "q4" );
	BeaconCheckTestMachine.add_edge( "q4", {']'}, "q5" );
	BeaconCheckTestMachine.add_edge( "q5", {' '}, "q6" );
	BeaconCheckTestMachine.add_edge( "q6", {' '}, "q6" );
	BeaconCheckTestMachine.add_edge( "q6", {','}, "q7" );
	BeaconCheckTestMachine.add_edge( "q5", {','}, "q7" );
	BeaconCheckTestMachine.add_edge( "q7", setAlpha, "q7" );
	BeaconCheckTestMachine.add_edge( "q7", setNumeric, "q7" );
	BeaconCheckTestMachine.add_edge( "q7", {'_',' ',',','+','-','^','*','.','(',')','/'}, "q7" );
	BeaconCheckTestMachine.add_edge( "q7", {'}'}, "endState" );

	/*accepts a beacon kill {channelName#[i], rate} */
	BeaconKillTestMachine.designate_endState( "endState" );
	BeaconKillTestMachine.add_edge( BeaconKillTestMachine.startState, {'{'}, "q1" );
	BeaconKillTestMachine.add_edge( "q1", {' '}, "q1" );

	BeaconKillTestMachine.add_edge( "q1", {'_',',','+','/','-','^','*','(',')','.'}, "q2" );
	BeaconKillTestMachine.add_edge( "q1", setAlpha, "q2" );
	BeaconKillTestMachine.add_edge( "q1", setNumeric, "q2" );

	BeaconKillTestMachine.add_edge( "q2", {'_',' ',',','+','/','-','^','*','(',')','.'}, "q2" );
	BeaconKillTestMachine.add_edge( "q2", setAlpha, "q2" );
	BeaconKillTestMachine.add_edge( "q2", setNumeric, "q2" );

	BeaconKillTestMachine.add_edge( "q2", {'#'}, "q3" );
	BeaconKillTestMachine.add_edge( "q3", {'['}, "q4" );
	BeaconKillTestMachine.add_edge( "q4", {'_',' ',',','+','/','-','*','^','.','(',')'}, "q4" );
	BeaconKillTestMachine.add_edge( "q4", setAlpha, "q4" );
	BeaconKillTestMachine.add_edge( "q4", setNumeric, "q4" );
	BeaconKillTestMachine.add_edge( "q4", {']'}, "q5" );
	BeaconKillTestMachine.add_edge( "q5", {' '}, "q6" );
	BeaconKillTestMachine.add_edge( "q6", {' '}, "q6" );
	BeaconKillTestMachine.add_edge( "q6", {','}, "q7" );
	BeaconKillTestMachine.add_edge( "q5", {','}, "q7" );
	BeaconKillTestMachine.add_edge( "q7", setAlpha, "q7" );
	BeaconKillTestMachine.add_edge( "q7", setNumeric, "q7" );
	BeaconKillTestMachine.add_edge( "q7", {'_',' ','^',',','+','-','*','.','(',')','/','"'}, "q7" );
	BeaconKillTestMachine.add_edge( "q7", {'}'}, "endState" );

	std::ifstream sourceFile( sourceFilename );

	if ( not sourceFile.is_open() ) throw BadSourcePath();

	std::vector< Token * > tokenisation;
	std::string line;
	unsigned int lineNumber = 0;
	unsigned int colNumber = 1;

	//tokenise each line
	while ( std::getline( sourceFile, line ) ){

		lineNumber++;

		/*strip out comments */
		line = line.substr( 0, line.find("//") );

		/*ignore lines that are empty or contain just whitespace */
		if ( line.empty() or line.find_first_not_of(' ') == std::string::npos ) continue;

		std::vector< Token * > tokenisedLine = scanLine( line, lineNumber, colNumber );

		tokenisation.insert( tokenisation.end(), tokenisedLine.begin(), tokenisedLine.end() );
	}
	sourceFile.close();

	//reshape into semicolon-terminated assignments and system line
	std::vector< std::vector< Token * > > parsedTokenisation;
	std::vector< Token * > running;
	for ( auto t = tokenisation.begin(); t < tokenisation.end(); t++ ){

		if ( (*t) -> identify() == "Semicolon" ){
			
			parsedTokenisation.push_back(running);
			running.clear();
		}
		else{

			running.push_back(*t);
		}
	}

	return parsedTokenisation;
};
