//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG_BLOCK_DEFS 1
//#define DEBUG_BLOCK_SYSTEMLINE 1
//#define DEBUG_BLOCK_ARITHMETIC 1

#include <set>
#include <algorithm>
#include <cassert>
#include <stack>
#include <locale>
#include "blockParser.h"
#include "error_handling.h"
#include "evaluate_trees.h"


void printBlockTree( Tree<Block> pt, Block *b ){
/*for testing/debugging - prints out the parse tree */

	/*if this token has children */
	if ( not pt.isLeaf( b ) ){

		std::vector< Block * > children = pt.getChildren( b );

		/*print them */
		std::cout << "Parent: " << b -> identify() << std::endl; 
		std::cout << "\tChildren: ";
	
		for ( auto child = children.begin(); child < children.end(); child++ ){

			std::cout << (*child) -> identify() << " ";
		}
		std::cout << std::endl;

		/*recurse on subtrees */
		for ( auto child = children.begin(); child < children.end(); child++ ){

			printBlockTree( pt, *child );
		}
	}

	if ( pt.isLeaf(b) and pt.isRoot(b) ) std::cout << "Lone node: " << b -> identify() << std::endl;

}


std::vector< std::vector< Token * > > splitOnCommas( std::vector< Token * > &tokenisedParam ){

	std::vector< std::vector< Token * > > allSplit;
	std::vector< Token * > buffer;	
	for ( auto t = tokenisedParam.begin(); t < tokenisedParam.end(); t++ ){

		if ( (*t) -> identify() != "Comma" ) buffer.push_back( *t );
		else{

			allSplit.push_back( buffer );
			buffer.clear();
		}
	}	
	allSplit.push_back( buffer );
	return allSplit;
}


/*BLOCK METHODS------------------------------------------------------------------------------------------------------------------------------------------------------*/
ActionBlock::ActionBlock( Token *t, std::string s ) : Block( t, s ){

	assert( t -> identify() == "Action" );
	_underlyingToken = t;
	_owningProcess = s;
	std::string wholeAction = t -> value();
	
	/*get the action name */
	std::string actionSubstr = wholeAction.substr(wholeAction.find("{")+1, wholeAction.find(",") - wholeAction.find("{") - 1);
	std::vector< Token * > tokenisedName = scanLine( actionSubstr, t -> getLine(), t -> getColumn() );
	if ( tokenisedName.size() != 1 ) throw SyntaxError( t, "Thrown by block parser: Action name must be parsed as one token." );
	actionName = tokenisedName[0] -> value();

	/*get the rate tokens and make a parse tree on arithmetic operations */
	std::string rateSubstr = wholeAction.substr(wholeAction.find(",")+1, wholeAction.find("}") - wholeAction.find(",") - 1);
	std::vector< Token * > tokenisedRate = scanLine( rateSubstr, t -> getLine(), t -> getColumn() );
	_RPNrate = shuntingYard( tokenisedRate );
}


ChoiceBlock::ChoiceBlock( Token *t, std::string s ) : Block( t, s ){

	assert( t -> value() == "+" );
	_owningProcess = s;
	_underlyingToken = t;
}


ParallelBlock::ParallelBlock( Token *t, std::string s ) : Block( t, s ){

	assert( t -> value() == "||" );
	_owningProcess = s;
	_underlyingToken = t;
}


GateBlock::GateBlock( Token *t, std::string s ) : Block( t, s ){

	assert( t -> identify() == "Gate" );
	_underlyingToken = t;
	_owningProcess = s;

	std::string wholeGate = t -> value();
	std::string betweenBrackets = wholeGate.substr( wholeGate.find("[") + 1, wholeGate.find("]") - wholeGate.find("[") - 1 );
	std::vector< Token * > tokenisedGate = scanLine( betweenBrackets, t -> getLine(), t -> getColumn() );
	_RPNexpression = shuntingYard( tokenisedGate );
}


MessageSendBlock::MessageSendBlock( Token *t, std::string s ) : Block( t, s ){

	std::set< std::string > acceptableTokens = { "BeaconKill", "MessageSend" };
	assert( acceptableTokens.find( t -> identify() ) != acceptableTokens.end() );
	_underlyingToken = t;
	_owningProcess = s;

	std::string wholeMessage = t -> value();
	std::string msgSubstr = wholeMessage.substr( wholeMessage.find("{") + 1, wholeMessage.find(",") - wholeMessage.find("{") - 1 );
	std::vector< Token * > tokenisedMessage = scanLine( msgSubstr, t -> getLine(), t -> getColumn() );
	std::string parameterExpression;

	/*beacon kill */
	if ( tokenisedMessage[1] -> value() == "#" ){

		if ( tokenisedMessage[0] -> identify() != "Variable" or tokenisedMessage[2] -> identify() != "ParameterCondition" ){

			throw SyntaxError( t, "Thrown by block parser: Message name and/or parameter condition is formatted incorrectly in beacon kill." );
		}
		_handshake = false;
		_kill = true;
		_channelName = tokenisedMessage[0] -> value();
		parameterExpression = tokenisedMessage[2] -> value();
	}
	/*beacon launch or handshake send */
	else{

		/*handshake */
		if ( tokenisedMessage[0] -> value() == "@" ){

			if ( tokenisedMessage[2] -> value() != "!" or tokenisedMessage[1] -> identify() != "Variable" ){

				throw SyntaxError( t, "Thrown by block parser: Bad channel name or '!' specifier missing from handshake send." );
			}
			if ( tokenisedMessage[3] -> identify() != "ParameterCondition" ){

				throw SyntaxError( t, "Thrown by block parser: Missing parameter condition or parameter condition is formatted incorrectly." );
			}
			_handshake = true;
			_kill = false;
			_channelName = tokenisedMessage[1] -> value();
			parameterExpression = tokenisedMessage[3] -> value();
		}
		/*beacon */
		else{

			if ( tokenisedMessage[1] -> value() != "!" or tokenisedMessage[0] -> identify() != "Variable" ){

				throw SyntaxError( t, "Thrown by block parser: Bad channel name or '!' specifier missing from beacon launch." );
			}
			if ( tokenisedMessage[2] -> identify() != "ParameterCondition" ){
		
				throw SyntaxError( t, "Thrown by block parser: Missing parameter condition or parameter condition is formatted incorrectly." );
			}
			_handshake = false;
			_kill = false;
			_channelName = tokenisedMessage[0] -> value();
			parameterExpression = tokenisedMessage[2] -> value();
		}
	}

	/*parse arithmetic expression on parameter */
	std::string betweenSquareBrackets = parameterExpression.substr( parameterExpression.find("[") + 1, parameterExpression.find("]") - parameterExpression.find("[") - 1 );
	std::vector< Token * > tokenisedParamArithmetic = scanLine( betweenSquareBrackets, t -> getLine(), t -> getColumn() );
	_RPNexpression = shuntingYard( tokenisedParamArithmetic );

	/*get the rate tokens and make a parse tree on arithmetic operations */
	std::string rateSubstr = wholeMessage.substr(wholeMessage.find(",")+1, wholeMessage.find("}") - wholeMessage.find(",") - 1);
	std::vector< Token * > tokenisedRate = scanLine( rateSubstr, t -> getLine(), t -> getColumn() );
	_RPNrate = shuntingYard( tokenisedRate );
}


MessageReceiveBlock::MessageReceiveBlock( Token *t, std::string s ) : Block( t, s ){

	std::set< std::string > acceptableTokens = { "BeaconCheck", "MessageReceive" };
	assert( acceptableTokens.find( t -> identify() ) != acceptableTokens.end() );
	_underlyingToken = t;
	_owningProcess = s;

	std::string wholeMessage = t -> value();
	std::string msgSubstr = wholeMessage.substr( wholeMessage.find("{") + 1, wholeMessage.find(",") - wholeMessage.find("{") - 1 );
	std::vector< Token * > tokenisedMessage = scanLine( msgSubstr, t -> getLine(), t -> getColumn() );
	std::string parameterExpression;

	/*beacon check */
	if ( tokenisedMessage[0] -> value() == "~" ){

		if ( tokenisedMessage[1] -> identify() != "Variable" or tokenisedMessage[3] -> identify() != "ParameterCondition" ){

			throw SyntaxError( t, "Thrown by block parser: Message name and/or parameter condition is formatted incorrectly." );
		}
		if ( tokenisedMessage[2] -> value() != "?" ){

			throw SyntaxError( t, "Thrown by block parser: Missing '?' specifier from beacon check." );
		}
		_handshake = false;
		_check = true;
		_channelName = tokenisedMessage[1] -> value();
		parameterExpression = tokenisedMessage[3] -> value();
	}
	/*beacon/handshake receive */
	else{

		/*handshake */
		if ( tokenisedMessage[0] -> value() == "@" ){

			if ( tokenisedMessage[2] -> value() != "?" or tokenisedMessage[1] -> identify() != "Variable" ){

				throw SyntaxError( t, "Thrown by block parser: Bad channel name or '?' specifier missing in handshake receive." );
			}
			if ( tokenisedMessage[3] -> identify() != "ParameterCondition" ){
			
				throw SyntaxError( t, "Thrown by block parser: Missing parameter condition or parameter condition is formatted incorrectly." );
			}
			_handshake = true;
			_check = false;
			_channelName = tokenisedMessage[1] -> value();
			parameterExpression = tokenisedMessage[3] -> value();
		}
		/*beacon */
		else{

			if ( tokenisedMessage[1] -> value() != "?" or tokenisedMessage[0] -> identify() != "Variable" ){

				throw SyntaxError( t, "Thrown by block parser: Bad channel name or '?' specifier missing in beacon receive." );
			}
			if ( tokenisedMessage[2] -> identify() != "ParameterCondition" ){
			
				throw SyntaxError( t, "Thrown by block parser: Missing parameter condition or parameter condition is formatted incorrectly." );
			}
			_handshake = false;
			_check = false;
			_channelName = tokenisedMessage[0] -> value();
			parameterExpression = tokenisedMessage[2] -> value();
		}
	}

	/*parse arithmetic/set operations to get the range of parameters */
	std::string betweenSquareBrackets = parameterExpression.substr( parameterExpression.find("[") + 1, parameterExpression.find("]") - parameterExpression.find("[") - 1 );
	std::vector< Token * > tokenisedParamArithmetic = scanLine( betweenSquareBrackets, t -> getLine(), t -> getColumn() );
	_RPNexpression = shuntingYard( tokenisedParamArithmetic );

	/*get the rate tokens and make a parse tree on arithmetic operations */
	std::string rateSubstr = wholeMessage.substr(wholeMessage.find(",")+1, wholeMessage.find("}") - wholeMessage.find(",") - 1);
	std::vector< Token * > tokenisedRate = scanLine( rateSubstr, t -> getLine(), t -> getColumn() );
	_RPNrate = shuntingYard( tokenisedRate );

	/*get binding variable, if any */
	std::string afterSquareBrackets = wholeMessage.substr( wholeMessage.find("]") + 1, wholeMessage.find(",") - wholeMessage.find("]") - 1 );
	std::vector< Token * > tokenisedBinding = scanLine( afterSquareBrackets, t -> getLine(), t -> getColumn() );
	if ( tokenisedBinding.size() > 0 ){

		_hasBindingVar = true;
		if ( tokenisedBinding.size() != 3 ){

			throw SyntaxError( t, "Thrown by block parser: Binding variable is formatted incorrectly." );
		}
		if ( tokenisedBinding[1] -> identify() != "Variable" or tokenisedBinding[0] -> identify() != "Parentheses" or tokenisedBinding[2] -> identify() != "Parentheses" ){
		
			throw SyntaxError( t, "Thrown by block parser: Binding variable is formatted incorrectly.");
		}
		_bindingVariable = tokenisedBinding[1] -> value();
	}

#if DEBUG_BLOCK_ARITHMETIC
std::cout << "-----------" << std::endl;
std::cout << _channelName << std::endl;
printTree( _setTree, _setTree.getRoot() );
printTree( _rateTree, _rateTree.getRoot() );
std::cout << "-----------" << std::endl;
#endif
}


ProcessBlock::ProcessBlock( Token *t, std::string s ) : Block( t, s ){

	assert( t -> identify() == "Process" );
	_underlyingToken = t;
	_owningProcess = s;
	std::string wholeProcess = t -> value();

	/*set parameter name */
	_processName = wholeProcess.substr(0, wholeProcess.find('['));

#if DEBUG_BLOCK_ARITHMETIC
std::cout << "-----------" << std::endl;
std::cout << _processName << std::endl;
#endif
	/*parse parameter arithmetic expression, if any */
	std::string betweenBrackets = wholeProcess.substr( wholeProcess.find("[") + 1, wholeProcess.find("]") - wholeProcess.find("[") - 1 );
	std::vector< Token * > tokenisedParam = scanLine( betweenBrackets, t -> getLine(), t -> getColumn() );
	if ( tokenisedParam.size() > 0 ){ //only bother doing this if we have parameters

		std::vector< std::vector< Token * > > split_tokenisedParam = splitOnCommas( tokenisedParam );
		for ( auto tv = split_tokenisedParam.begin(); tv < split_tokenisedParam.end(); tv++ ){

			_parameterExpressions.push_back( shuntingYard( *tv ) );
		}
	}
}


/*SECOND PASS PARSING FUNCTIONS--------------------------------------------------------------------------------------------------------------------------------------*/
Block *tokenToBlock( Token *t, std::string processName ){

	Block *newBlock;
	
	if ( t -> identify() == "Action" ){

		newBlock = new ActionBlock( t, processName );
	}
	else if ( t -> value() == "+" ){

		newBlock = new ChoiceBlock( t, processName );
	}
	else if ( t -> value() == "||" ){

		newBlock = new ParallelBlock( t, processName );
	}
	else if ( t -> identify() == "Gate" ){

		newBlock = new GateBlock( t, processName );
	}
	else if ( t -> identify() == "MessageSend" or t -> identify() == "BeaconKill" ){

		newBlock = new MessageSendBlock( t, processName );
	}
	else if ( t -> identify() == "MessageReceive" or t -> identify() == "BeaconCheck" ){

		newBlock = new MessageReceiveBlock( t, processName );
	}
	else if ( t -> identify() == "Process" ){

		newBlock = new ProcessBlock( t, processName );
	}
	else assert(false);

	return newBlock;
}


void secondParseSystemLine( std::vector< Token * > &tokenisedSL, std::list< SystemProcess > &system, std::map< std::string, ProcessDefinition > &processName2Definition, GlobalVariables &globalVars ){

	/*last entry in the system line should be a process */
	if ( (*(tokenisedSL.end() - 1)) -> identify() != "Process" ){

		throw SyntaxError( *(tokenisedSL.end() - 1), "Thrown by block parser: Trailing token in system line must be a process.");
	}

	/*make sure all operators are ||, that each process is separated by ||, and that all processes have a corresponding definition */
	int multiplier = 1;
	for ( auto t = tokenisedSL.begin(); t < tokenisedSL.end(); t++ ){

		if( (*t) -> identify() == "Process" ){

			SystemProcess sp;

			/*get the name of the process */
			std::string wholeProcess = (*t) -> value();
			std::string processName = wholeProcess.substr( 0, wholeProcess.find('[') );

			/*set the readhead at the process tree's root */
			if ( processName2Definition.count( processName ) == 0 ) throw UndefinedVariable( *t );
			
			sp.parseTree = (processName2Definition[processName]).parseTree;
			
			/*get the initial conditions of the parameters */
			std::string betweenBrackets = wholeProcess.substr( wholeProcess.find("[") + 1, wholeProcess.find("]") - wholeProcess.find("[") - 1 );
			std::vector< Token * > tokenisedParams = scanLine( betweenBrackets, (*t) -> getLine(), (*t) -> getColumn() );

			std::vector< std::string > parameterVar = (processName2Definition[processName]).parameters;

			if (tokenisedParams.size() == 0){ //if we don't have any process parameters, check this matches the definition and move on

				if (parameterVar.size() != 0){
					throw SyntaxError( *t, "Thrown by block parser: Number of parameters specified do not match the process definition." );
				}
			}
			else{ //if you do have parameters, check they match the definition and update the system process accordingly

				std::vector< std::vector< Token * > > split_tokenisedParam = splitOnCommas( tokenisedParams );
				if (parameterVar.size() != split_tokenisedParam.size()) throw SyntaxError( *t, "Thrown by block parser: Number of parameters specified do not match the process definition." );
				ParameterValues pValues;
				for ( unsigned int i = 0; i < split_tokenisedParam.size(); i++ ){

					if ( split_tokenisedParam[i].size() != 1 ) throw SyntaxError( *t, "Thrown by block parser: Parameter value must be a defined variable, int literal, or float literal." );
					std::string parameterValue = split_tokenisedParam[i][0] -> value();
					if (split_tokenisedParam[i][0] -> identify() == "IntLiteral"){
						pValues.updateValue(parameterVar[i], atoi( parameterValue.c_str()));
					}
					else if (split_tokenisedParam[i][0] -> identify() == "DoubleLiteral"){
						pValues.updateValue(parameterVar[i], atof( parameterValue.c_str()));
					}
					else throw SyntaxError(split_tokenisedParam[i][0], "Thrown by block parser: Parameters must be int literals or float literals.");
				}
				sp.parameterValues = pValues;
			}
			
			//add multiple copies to the system if there was a multiplier on the system line
			for ( int i = 0; i < multiplier; i++ ){

				system.push_back( sp );
			}

#if DEBUG_BLOCK_SYSTEMLINE
std::cout << processName << std::endl;
std::cout << "copies: " << multiplier << std::endl << "parse tree:" << std::endl;
printBlockTree( sp.parseTree, sp.parseTree.getRoot() );
std::cout << "ints:" << std::endl;
for (auto param = sp.parameterValues.intValues.begin(); param != sp.parameterValues.intValues.end(); param++ ){
	std::cout << param -> first << " " << param -> second << std::endl;
}
std::cout << "doubles:" << std::endl;
for (auto param = sp.parameterValues.doubleValues.begin(); param != sp.parameterValues.doubleValues.end(); param++ ){
	std::cout << param -> first << " " << param -> second << std::endl;
}
#endif
			multiplier = 1;
		}
		else if ( (*t) -> identify() == "Variable" or (*t) -> identify() == "IntLiteral" ){

			std::string str_multiplier = (*t) -> value();
			
			if ( (*t) -> identify() == "Variable" ){

				if ( globalVars.doubleValues.count(str_multiplier) == 0 and globalVars.intValues.count(str_multiplier) == 0 ) throw UndefinedVariable( *t );
				if ( globalVars.doubleValues.count(str_multiplier) > 0 ) throw SyntaxError( *t, "Thrown by block parser: System process multiplier must be an int, not a float.");
				multiplier = globalVars.intValues.at( str_multiplier );
			}
			else multiplier = std::stoi(str_multiplier);

			if ( multiplier <= 0 ) throw SyntaxError(*t, "Thrown by block parser: System process multiplier must be greater than zero.");
		}
	}
}


void secondParseProcessDef( Token *t, Block *b, Tree<Token> &treeForLine, Tree<Block> &bt, std::string processName ){
//recursively iterates down a parse tree and builds a block tree with the same structure
//arguments:
// - t: the parent token,
// - b: the parent block,
// - treeForLine: the parse tree from the first round of parsing,
// - bt: the block tree that we're going to build.

	if ( not treeForLine.isLeaf( t ) ){
	
		std::vector< Token * > children = treeForLine.getChildren( t );

		for ( auto c = children.begin(); c < children.end(); c++ ){

			Block *newChild = tokenToBlock( *c, processName );
			bt.addChild( b, newChild );
			secondParseProcessDef( *c, newChild, treeForLine, bt, processName );
		}
	}
}


void checkProcessDefinition( Block *b, Tree<Block> &bt, std::map< std::string, ProcessDefinition > &processName2Definition ){
//checks that if a process block is used in a process definition, that process block has the right number of parameters

	if ( b -> identify() == "Process" ){

		ProcessBlock *pb = dynamic_cast< ProcessBlock* >(b);
		std::vector< std::vector<Token*> > parameterExpressions = pb -> getParameterExpressions();
		if (parameterExpressions.size() != processName2Definition[pb -> getProcessName()].parameters.size()){

			throw SyntaxError( b -> getToken(), "Thrown by block parser: Number of parameters specified do not match the process definition." );
		}
	}

	if ( not bt.isLeaf( b ) ){
	
		std::vector< Block * > children = bt.getChildren( b );

		for ( auto c = children.begin(); c < children.end(); c++ ){

			checkProcessDefinition( *c, bt, processName2Definition );
		}
	}
}


std::pair< std::map< std::string, ProcessDefinition >, std::list< SystemProcess > > secondPassParse( std::vector< Tree<Token> > processDefPTs,
							                            std::vector< Token* > tokenisedSystemLine,
										    GlobalVariables &globalVars ){
//main function for second pass parsing.  sets the root of the new block tree, calls
//secondParseProcessDef to fill out the tree, then substitutes all variables 
//arguments:
// - processDefPTs: the parse trees from the first round of parsing, just process and variable definitions

	/*second round parse of process definitions */
	std::map< std::string, ProcessDefinition > processName2Definition;

	for ( auto pt = processDefPTs.begin(); pt < processDefPTs.end(); pt++ ){

		ProcessDefinition pd;

		/*get the process name */
		std::vector< Token * > children = pt -> getChildren( pt -> getRoot() );
		std::string pTokenValue = children[0] -> value();
		std::string processName = pTokenValue.substr(0, pTokenValue.find('['));
		
		/*get the process parameters */
		std::string betweenBrackets = pTokenValue.substr( pTokenValue.find("[") + 1, pTokenValue.find("]") - pTokenValue.find("[") - 1 );
		std::vector< Token * > tokenisedParam = scanLine( betweenBrackets, children[0] -> getLine(), children[0] -> getColumn() );
		if ( tokenisedParam.size() > 0 ){

			if ( tokenisedParam.back() -> identify() != "Variable" ) throw SyntaxError( tokenisedParam.back(), "Thrown by block parser: Parameter list must trail with a variable." );
			int flip = 0;
			
			for ( auto t = tokenisedParam.begin(); t < tokenisedParam.end(); t++ ){
				if ( (*t) -> identify() == "Variable" and flip == 0 ){

					flip++; flip %= 2;
					(pd.parameters).push_back( (*t) -> value() );
					continue;
				} 
				else if ( (*t) -> identify() == "Comma" and flip == 1 ){

					flip++; flip %= 2;
					continue;
				}
				else throw SyntaxError( *t, "Thrown by block parser: Unrecognised token in parameter list - must be a variable or a comma." );
			}
		}

		/*root the new block tree, recurse on the token tree to fill it up, and associate it to the process name */
		Block *root = tokenToBlock( children[1], processName );
		(pd.parseTree).setRoot( root );
		secondParseProcessDef( children[1], root, *pt, pd.parseTree, processName );
		processName2Definition[ processName ] = pd;

#if DEBUG_BLOCK_DEFS
std::cout << processName << std::endl;
printBlockTree( pd.parseTree, (pd.parseTree).getRoot() );
std::cout << "-----------" << std::endl;
#endif
	}

	//check the processes that we've defined
	for ( auto pd = processName2Definition.begin(); pd != processName2Definition.end(); pd++ ){

		checkProcessDefinition( (pd -> second).parseTree.getRoot(), (pd -> second).parseTree, processName2Definition );
	}

	/*second round parse of system line */
	std::list< SystemProcess > system;
	secondParseSystemLine( tokenisedSystemLine, system, processName2Definition, globalVars );

#if defined DEBUG_BLOCK_DEFS || defined DEBUG_BLOCK_SYSTEMLINE || defined DEBUG_BLOCK_ARITHMETIC
exit(EXIT_SUCCESS);
#endif
	return std::make_pair(processName2Definition, system );
}
