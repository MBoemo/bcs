//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG 1

#include <set>
#include <algorithm>
#include <cassert>
#include <stack>
#include <locale>
#include "blockParser.h"
#include "error_handling.h"
#include "evaluate_trees.h"

bool operator== (const ParameterValues &pv1, const ParameterValues &pv2){

	if (pv1.values == pv2.values) return true;
	else return false;
}

bool operator!= (const ParameterValues &pv1, const ParameterValues &pv2){

	return !(pv1 == pv2);
}

bool operator== (const SystemProcess &sp1, const SystemProcess &sp2){

	if (sp1.parseTree == sp2.parseTree && sp1.parameterValues == sp2.parameterValues && sp1.localVariables == sp2.localVariables) return true;
	else return false;
}

bool operator!= (const SystemProcess &sp1, const SystemProcess &sp2){

	return !(sp1 == sp2);
}

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
	std::stack< Token * > parenStack;
	for ( auto t = tokenisedParam.begin(); t < tokenisedParam.end(); t++ ){

		if ( (*t) -> value() == "(" ) parenStack.push( *t );
		else if ( (*t) -> value() == ")" ) parenStack.pop();

		if ( (*t) -> identify() == "Comma" and parenStack.empty() ) {

			if (buffer.size() == 0) throw SyntaxError(*t, "Thrown by block parser: Comma-separated list cannot contain the empty string.");
			allSplit.push_back( buffer );
			buffer.clear();
		}
		else buffer.push_back( *t );
	}	
	allSplit.push_back( buffer );
	return allSplit;
}


/*BLOCK METHODS------------------------------------------------------------------------------------------------------------------------------------------------------*/
ActionBlock::ActionBlock( Token *t, std::string s, std::vector<std::string> parameterNames, std::vector<std::string> globalVarNames ) : Block( t, s, parameterNames, globalVarNames ){

#if DEBUG
std::cout << "---------------" << std::endl;
std::cout << "Starting ActionBlock parsing on: " << t -> value() << std::endl;
#endif

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
	for (auto tr = tokenisedRate.begin(); tr < tokenisedRate.end(); tr++){
		if ((*tr) -> identify() == "Variable"){
			std::string variableName = (*tr) -> value();
			if (std::find(parameterNames.begin(),parameterNames.end(),variableName) == parameterNames.end()
				and std::find(globalVarNames.begin(),globalVarNames.end(),variableName) == globalVarNames.end()){
					throw UndefinedVariable(*tr);
				}
		}
	}
	_RPNrate = shuntingYard( tokenisedRate );


#if DEBUG
std::cout << "Tokenised rate in RPN: ";
for ( auto t = _RPNrate.begin(); t < _RPNrate.end(); t++ ) std::cout << (*t) -> value() << " ";
std::cout << std::endl;
#endif
}


ChoiceBlock::ChoiceBlock( Token *t, std::string s, std::vector<std::string> parameterNames, std::vector<std::string> globalVarNames ) : Block( t, s, parameterNames, globalVarNames ){

	assert( t -> value() == "+" );
	_owningProcess = s;
	_underlyingToken = t;
}


ParallelBlock::ParallelBlock( Token *t, std::string s, std::vector<std::string> parameterNames, std::vector<std::string> globalVarNames ) : Block( t, s, parameterNames, globalVarNames ){

	assert( t -> value() == "||" );
	_owningProcess = s;
	_underlyingToken = t;
}


GateBlock::GateBlock( Token *t, std::string s, std::vector<std::string> parameterNames, std::vector<std::string> globalVarNames ) : Block( t, s, parameterNames, globalVarNames ){

#if DEBUG
std::cout << "---------------" << std::endl;
std::cout << "Starting GateBlock parsing on: " << t -> value() << std::endl;
#endif

	assert( t -> identify() == "Gate" );
	_underlyingToken = t;
	_owningProcess = s;

	std::string wholeGate = t -> value();
	std::string betweenBrackets = wholeGate.substr( wholeGate.find("[") + 1, wholeGate.find("]") - wholeGate.find("[") - 1 );
	std::vector< Token * > tokenisedGate = scanLine( betweenBrackets, t -> getLine(), t -> getColumn() );

	if (tokenisedGate.size() == 0) throw SyntaxError( t, "Gate condition cannot be empty.");

	for (auto tr = tokenisedGate.begin(); tr < tokenisedGate.end(); tr++){
		if ((*tr) -> identify() == "Variable"){
			std::string variableName = (*tr) -> value();
			if (std::find(parameterNames.begin(),parameterNames.end(),variableName) == parameterNames.end()
				and std::find(globalVarNames.begin(),globalVarNames.end(),variableName) == globalVarNames.end()){
					throw UndefinedVariable(*tr);
				}
		}
	}

	_RPNexpression = shuntingYard( tokenisedGate );

#if DEBUG
std::cout << "Tokenised condition in RPN: ";
for ( auto t = _RPNexpression.begin(); t < _RPNexpression.end(); t++ ) std::cout << (*t) -> value() << " ";
std::cout << std::endl;
#endif
}


MessageSendBlock::MessageSendBlock( Token *t, std::string s, std::vector<std::string> parameterNames, std::vector<std::string> globalVarNames ) : Block( t, s, parameterNames, globalVarNames ){

#if DEBUG
std::cout << "---------------" << std::endl;
std::cout << "Starting MessageSendBlock parsing on: " << t -> value() << std::endl;
#endif

	std::set< std::string > acceptableTokens = { "BeaconKill", "MessageSend" };
	assert( acceptableTokens.find( t -> identify() ) != acceptableTokens.end() );
	_underlyingToken = t;
	_owningProcess = s;

	std::string wholeMessage = t -> value();
	std::string chanSubstr = wholeMessage.substr( wholeMessage.find("{") + 1, wholeMessage.find("[") - wholeMessage.find("{") - 1 );
	std::vector< Token * > tokenisedChannel = scanLine( chanSubstr, t -> getLine(), t -> getColumn() );
	std::vector< Token * > buffer;

	//parse channel names
	if (wholeMessage.find('#') != std::string::npos){/*beacon kill */

		tokenisedChannel.pop_back(); //remove # character from end
		_handshake = false;
		_kill = true;
	}
	else if (wholeMessage.find('@') != std::string::npos){/*handshake send */

		tokenisedChannel.pop_back(); //remove ! character from the end
		tokenisedChannel.erase(tokenisedChannel.begin()); //remove @ character from beginning
		_handshake = true;
		_kill = false;
	}
	else{/*beacon */

		tokenisedChannel.pop_back(); //remove ! character from the end
		_handshake = false;
		_kill = false;
	}
	_channelNames = splitOnCommas( tokenisedChannel );
	for ( unsigned int i = 0; i < _channelNames.size(); i++ ) _channelNames[i] = shuntingYard(_channelNames[i]);

#if DEBUG
std::cout << "Type of send: ";
if (_handshake) std::cout << "HANDSHAKE" << std::endl;
else if (_kill) std::cout << "BEACON KILL" << std::endl;
else std::cout << "BEACON SEND" << std::endl;
for ( auto exp = _channelNames.begin(); exp < _channelNames.end(); exp++ ){

	std::cout << "Channel name expression:" << std::endl;
	for ( auto t = (*exp).begin(); t < (*exp).end(); t++ ){
		std::cout << (*t) -> value() << std::endl;
	}
}
#endif


	//parse parameters
	buffer.clear();
	std::string betweenSquareBrackets = wholeMessage.substr( wholeMessage.find("[") + 1, wholeMessage.find("]") - wholeMessage.find("[") - 1 );
	std::vector< Token * > tokenisedParamArithmetic = scanLine( betweenSquareBrackets, t -> getLine(), t -> getColumn() );
	for (auto tr = tokenisedParamArithmetic.begin(); tr < tokenisedParamArithmetic.end(); tr++){
		if ((*tr) -> identify() == "Variable"){
			std::string variableName = (*tr) -> value();
			if (std::find(parameterNames.begin(),parameterNames.end(),variableName) == parameterNames.end()
				and std::find(globalVarNames.begin(),globalVarNames.end(),variableName) == globalVarNames.end()){
					throw UndefinedVariable(*tr);
				}
		}
	}
	_RPNexpressions = splitOnCommas( tokenisedParamArithmetic );
	
	if (tokenisedParamArithmetic.size() == 0) throw SyntaxError( t, "Message must send a comma-separated list of at least one value.");

	for ( unsigned int i = 0; i < _RPNexpressions.size(); i++ ) _RPNexpressions[i] = shuntingYard(_RPNexpressions[i]);

#if DEBUG
for ( auto exp = _RPNexpressions.begin(); exp < _RPNexpressions.end(); exp++ ){

	std::cout << "Parameter expression:" << std::endl;
	for ( auto t = (*exp).begin(); t < (*exp).end(); t++ ){
		std::cout << (*t) -> value() << std::endl;
	}
}
#endif
		
	//get the rate tokens and make a parse tree on arithmetic operations
	std::string rateSubstr = wholeMessage.substr(wholeMessage.find(",",wholeMessage.find(']'))+1, wholeMessage.find("}") - wholeMessage.find(",",wholeMessage.find(']')) - 1);
	std::vector< Token * > tokenisedRate = scanLine( rateSubstr, t -> getLine(), t -> getColumn() );
	for (auto tr = tokenisedRate.begin(); tr < tokenisedRate.end(); tr++){
		if ((*tr) -> identify() == "Variable"){
			std::string variableName = (*tr) -> value();
			if (std::find(parameterNames.begin(),parameterNames.end(),variableName) == parameterNames.end()
				and std::find(globalVarNames.begin(),globalVarNames.end(),variableName) == globalVarNames.end()){
					throw UndefinedVariable(*tr);
				}
		}
	}
	_RPNrate = shuntingYard( tokenisedRate );

#if DEBUG
std::cout << "Tokenised rate in RPN: ";
for ( auto t = _RPNrate.begin(); t < _RPNrate.end(); t++ ) std::cout << (*t) -> value() << " ";
std::cout << std::endl;
#endif
}


MessageReceiveBlock::MessageReceiveBlock( Token *t, std::string s, std::vector<std::string> parameterNames, std::vector<std::string> globalVarNames ) : Block( t, s, parameterNames, globalVarNames ){

#if DEBUG
std::cout << "---------------" << std::endl;
std::cout << "Starting MessageReceiveBlock parsing on: " << t -> value() << std::endl;
#endif

	std::set< std::string > acceptableTokens = {"BeaconCheck", "MessageReceive"};
	assert( acceptableTokens.find( t -> identify() ) != acceptableTokens.end() );
	_underlyingToken = t;
	_owningProcess = s;

	std::string wholeMessage = t -> value();
	std::string chanSubstr = wholeMessage.substr( wholeMessage.find("{") + 1, wholeMessage.find("[") - wholeMessage.find("{") - 1 );
	std::vector< Token * > tokenisedChannel = scanLine( chanSubstr, t -> getLine(), t -> getColumn() );
	std::vector< Token * > buffer;

	//parse channel names
	if (wholeMessage.find('~') != std::string::npos){/*beacon check */

		tokenisedChannel.erase(tokenisedChannel.begin()); //remove ~ character from beginning
		tokenisedChannel.pop_back(); //remove ? character from the end
		_handshake = false;
		_check = true;
	}
	else if (wholeMessage.find('@') != std::string::npos){/*handshake send */

		tokenisedChannel.erase(tokenisedChannel.begin()); //remove @ character from beginning
		tokenisedChannel.pop_back(); //remove ? character from the end
		_handshake = true;
		_check = false;
	}
	else{/*beacon */

		tokenisedChannel.pop_back(); //remove ? character from the end
		_handshake = false;
		_check = false;
	}
	_channelNames = splitOnCommas( tokenisedChannel );
	for ( unsigned int i = 0; i < _channelNames.size(); i++ ) _channelNames[i] = shuntingYard(_channelNames[i]);

#if DEBUG
std::cout << "Type of receive: ";
if (_handshake) std::cout << "HANDSHAKE" << std::endl;
else if (_check) std::cout << "BEACON CHECK" << std::endl;
else std::cout << "BEACON RECEIVE" << std::endl;
for ( auto exp = _channelNames.begin(); exp < _channelNames.end(); exp++ ){

	std::cout << "Channel name expression:" << std::endl;
	for ( auto t = (*exp).begin(); t < (*exp).end(); t++ ){
		std::cout << (*t) -> value() << std::endl;
	}
}
#endif

	//parse parameters
	std::string betweenSquareBrackets = wholeMessage.substr( wholeMessage.find("[") + 1, wholeMessage.find("]") - wholeMessage.find("[") - 1 );
	std::vector< Token * > tokenisedParamArithmetic = scanLine( betweenSquareBrackets, t -> getLine(), t -> getColumn() );
	for (auto tr = tokenisedParamArithmetic.begin(); tr < tokenisedParamArithmetic.end(); tr++){
		if ((*tr) -> identify() == "Variable"){
			std::string variableName = (*tr) -> value();
			if (std::find(parameterNames.begin(),parameterNames.end(),variableName) == parameterNames.end()
				and std::find(globalVarNames.begin(),globalVarNames.end(),variableName) == globalVarNames.end()){
					throw UndefinedVariable(*tr);
				}
		}
	}
	_RPNexpressions = splitOnCommas( tokenisedParamArithmetic );

	//check if it uses set operations so we can optimise it if not
	for (auto paramToken = tokenisedParamArithmetic.begin(); paramToken < tokenisedParamArithmetic.end(); paramToken++){

		if ( (*paramToken) -> identify() == "SetOperation" ){
			_usesSets = true;
			break;
		}
	}

	if (tokenisedParamArithmetic.size() == 0) throw SyntaxError( t, "Message must receive comma-separated list of at least one value or set.");

	for ( unsigned int i = 0; i < _RPNexpressions.size(); i++ ) _RPNexpressions[i] = shuntingYard(_RPNexpressions[i]);

#if DEBUG
for ( auto exp = _RPNexpressions.begin(); exp < _RPNexpressions.end(); exp++ ){

	std::cout << "Parameter expression:" << std::endl;
	for ( auto t = (*exp).begin(); t < (*exp).end(); t++ ){
		std::cout << (*t) -> value() << std::endl;
	}
}
#endif

	//check if we bind a variable
	std::string betweenCurlyBrackets = wholeMessage.substr( wholeMessage.find("{") + 1, wholeMessage.find("}") - wholeMessage.find("{") - 1 );
	std::vector< Token * > tokenisedWholeMessage = scanLine( betweenCurlyBrackets, t -> getLine(), t -> getColumn() );
	bool foundLeft = false;
	std::vector< Token * > tokenisedRate, tokenisedBinding;
	for (auto t = tokenisedWholeMessage.begin(); t < tokenisedWholeMessage.end(); t++){

		if ( (*t) -> identify() == "ParameterCondition" ){
			t++;
			if ( (*t) -> value() == "(" ){
				_hasBindingVar = true;
				foundLeft = true;
				tokenisedBinding.push_back(*t);
			}
		}
		else if (foundLeft and (*t) -> value() == ")" and (*(t+1)) -> value() == ","){
			tokenisedBinding.push_back(*t);
			t += 2;
			tokenisedRate.insert(tokenisedRate.end(),t,tokenisedWholeMessage.end());
			break;
		}
		else if (foundLeft) tokenisedBinding.push_back(*t);	
	}
		
#if DEBUG
std::cout << "Binds variable? " << _hasBindingVar << std::endl;
#endif

	//parse the binding variables if there are any
	if (_hasBindingVar){

		if ( (tokenisedBinding[0] -> value()) != "(" or (tokenisedBinding.back() -> value()) != ")"){

			throw SyntaxError( tokenisedBinding[0], "Binding variables must be enclosed in parentheses.");
		}

		for ( unsigned int i = 1; i < tokenisedBinding.size()-1; i++ ){ //go through list of binding variables

#if DEBUG
std::cout << "Binding variable token: " << tokenisedBinding[i] -> value() << std::endl;
#endif

			if ( tokenisedBinding[i] -> identify() == "Comma" ){

				if (i % 2 != 0) throw SyntaxError( tokenisedBinding[i], "Binding variables must be a comma-separated list of variables.");	
			}
			else if (tokenisedBinding[i] -> identify() == "Variable"){

				if (i % 2 != 1) throw SyntaxError( tokenisedBinding[i], "Binding variables must be a comma-separated list of variables.");	
				_bindingVariables.push_back( tokenisedBinding[i] -> value() );
			}
			else{

				throw SyntaxError( tokenisedBinding[i], "Binding variables must be a comma-separated list of variables.");	
			}
		}
		
		//we should have a binding variable for each parameter in the receive action
		if (_bindingVariables.size() != _RPNexpressions.size()){

			throw SyntaxError( t, "Binding variables must be a comma-separated list of variables.");	
		}

		//if we bind a variable, we already figured out what the tokenised rate is above
		for (auto tr = tokenisedRate.begin(); tr < tokenisedRate.end(); tr++){
			if ((*tr) -> identify() == "Variable"){
				std::string variableName = (*tr) -> value();
				if (std::find(parameterNames.begin(),parameterNames.end(),variableName) == parameterNames.end()
					and std::find(globalVarNames.begin(),globalVarNames.end(),variableName) == globalVarNames.end()
					and std::find(_bindingVariables.begin(),_bindingVariables.end(),variableName) == _bindingVariables.end()){
						throw UndefinedVariable(*tr);
					}
			}
		}
		_RPNrate = shuntingYard( tokenisedRate );
	}
	else{//if no binding variable, parse the rate the same way we would for a send

		//get the rate tokens and make a parse tree on arithmetic operations
		std::string rateSubstr = wholeMessage.substr(wholeMessage.find(",",wholeMessage.find(']'))+1, wholeMessage.find("}") - wholeMessage.find(",",wholeMessage.find(']')) - 1);
		tokenisedRate = scanLine( rateSubstr, t -> getLine(), t -> getColumn() );
		for (auto tr = tokenisedRate.begin(); tr < tokenisedRate.end(); tr++){
			if ((*tr) -> identify() == "Variable"){
				std::string variableName = (*tr) -> value();
				if (std::find(parameterNames.begin(),parameterNames.end(),variableName) == parameterNames.end()
					and std::find(globalVarNames.begin(),globalVarNames.end(),variableName) == globalVarNames.end()){
						throw UndefinedVariable(*tr);
					}
			}
		}
		_RPNrate = shuntingYard( tokenisedRate );
	}

#if DEBUG
std::cout << "Tokenised rate in RPN: ";
for ( auto t = _RPNrate.begin(); t < _RPNrate.end(); t++ ) std::cout << (*t) -> value() << " ";
std::cout << std::endl;
#endif
}


ProcessBlock::ProcessBlock( Token *t, std::string s, std::vector<std::string> parameterNames, std::vector<std::string> globalVarNames ) : Block( t, s, parameterNames, globalVarNames ){

#if DEBUG
std::cout << "---------------" << std::endl;
std::cout << "Starting MessageReceiveBlock parsing on: " << t -> value() << std::endl;
#endif

	assert( t -> identify() == "Process" );
	_underlyingToken = t;
	_owningProcess = s;
	std::string wholeProcess = t -> value();

	/*set parameter name */
	_processName = wholeProcess.substr(0, wholeProcess.find('['));

	/*parse parameter arithmetic expression, if any */
	std::string betweenBrackets = wholeProcess.substr( wholeProcess.find("[") + 1, wholeProcess.find("]") - wholeProcess.find("[") - 1 );
	std::vector< Token * > tokenisedParam = scanLine( betweenBrackets, t -> getLine(), t -> getColumn() );
	for (auto tr = tokenisedParam.begin(); tr < tokenisedParam.end(); tr++){
		if ((*tr) -> identify() == "Variable"){
			std::string variableName = (*tr) -> value();
			if (std::find(parameterNames.begin(),parameterNames.end(),variableName) == parameterNames.end()
				and std::find(globalVarNames.begin(),globalVarNames.end(),variableName) == globalVarNames.end()){
					throw UndefinedVariable(*tr);
				}
		}
	}
	if ( tokenisedParam.size() > 0 ){ //only bother doing this if we have parameters

		std::vector< std::vector< Token * > > split_tokenisedParam = splitOnCommas( tokenisedParam );
		for ( auto tv = split_tokenisedParam.begin(); tv < split_tokenisedParam.end(); tv++ ){

			_parameterExpressions.push_back( shuntingYard( *tv ) );
		}
	}
#if DEBUG
for ( auto exp = _parameterExpressions.begin(); exp < _parameterExpressions.end(); exp++ ){

	std::cout << "Parameter expression:" << std::endl;
	for ( auto t = (*exp).begin(); t < (*exp).end(); t++ ){
		std::cout << (*t) -> value() << std::endl;
	}
}
#endif
}


/*SECOND PASS PARSING FUNCTIONS--------------------------------------------------------------------------------------------------------------------------------------*/
Block *tokenToBlock( Token *t,
		             std::string processName,
					 std::vector<std::string> parameterNames,
					 std::vector<std::string> globalVarNames ){

	Block *newBlock;
	
	if ( t -> identify() == "Action" ){

		newBlock = new ActionBlock( t, processName, parameterNames, globalVarNames );
	}
	else if ( t -> value() == "+" ){

		newBlock = new ChoiceBlock( t, processName, parameterNames, globalVarNames );
	}
	else if ( t -> value() == "||" ){

		newBlock = new ParallelBlock( t, processName, parameterNames, globalVarNames );
	}
	else if ( t -> identify() == "Gate" ){

		newBlock = new GateBlock( t, processName, parameterNames, globalVarNames );
	}
	else if ( t -> identify() == "MessageSend" or t -> identify() == "BeaconKill" ){

		newBlock = new MessageSendBlock( t, processName, parameterNames, globalVarNames );
	}
	else if ( t -> identify() == "MessageReceive" or t -> identify() == "BeaconCheck" ){

		newBlock = new MessageReceiveBlock( t, processName, parameterNames, globalVarNames );
	}
	else if ( t -> identify() == "Process" ){

		newBlock = new ProcessBlock( t, processName, parameterNames, globalVarNames );
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

					std::vector< Token * > parsedIntlExp = shuntingYard( split_tokenisedParam[i] );
					ParameterValues ParameterValues_dummy;
					std::map< std::string, Numerical > localVariables_dummy;
					Numerical intlValue = evalRPN_numerical(parsedIntlExp, ParameterValues_dummy, globalVars, localVariables_dummy);
					pValues.updateValue(parameterVar[i], intlValue);
				}
				sp.parameterValues = pValues;
			}
			
			sp.clones = multiplier;
			system.push_back(sp);

#if DEBUG
std::cout << "---------------" << std::endl;
std::cout << "SYSTEM LINE PARSE:" << std::endl;
std::cout << "Process name: " << processName << std::endl;
std::cout << "copies: " << multiplier << std::endl << "parse tree:" << std::endl;
printBlockTree( sp.parseTree, sp.parseTree.getRoot() );
std::cout << "ints:" << std::endl;
for (auto param = sp.parameterValues.values.begin(); param != sp.parameterValues.values.end(); param++ ){
	
	if ( (param -> second).isDouble() ) std::cout << param -> first << " " << (param -> second).getDouble() << " double" << std::endl;
	if ( (param -> second).isInt() ) std::cout << param -> first << " " << (param -> second).getInt() << " int" << std::endl;
}
#endif
			multiplier = 1;
		}
		else if ( (*t) -> identify() == "Variable" or (*t) -> identify() == "IntLiteral" ){

			std::string str_multiplier = (*t) -> value();
			
			if ( (*t) -> identify() == "Variable" ){

				if ( globalVars.values.count(str_multiplier) == 0 ) throw UndefinedVariable( *t );
				Numerical multiplier_n = globalVars.values.at(str_multiplier);
				if (multiplier_n.isDouble()) throw SyntaxError( *t, "Thrown by block parser: System process multiplier must be an int, not a float.");
				multiplier = multiplier_n.getInt();
			}
			else{

				multiplier = std::stoi(str_multiplier);
			}


			if ( multiplier <= 0 ) throw SyntaxError(*t, "Thrown by block parser: System process multiplier must be greater than zero.");
		}
	}
}


void secondParseProcessDef( Token *t, Block *b,
		                    Tree<Token> &treeForLine,
							Tree<Block> &bt,
							std::string processName,
							std::vector<std::string> parameterNames,
							std::vector<std::string> globalVarNames ){
//recursively iterates down a parse tree and builds a block tree with the same structure
//arguments:
// - t: the parent token,
// - b: the parent block,
// - treeForLine: the parse tree from the first round of parsing,
// - bt: the block tree that we're going to build.
// - parameterNames: parameter variable names so we can check all variables are defined
// - globalVars: global variable names so we can check all variables are defined

	/*carry forward new binding variables if we have them */
	if (b -> identify() == "MessageReceive"){
		MessageReceiveBlock *receive = dynamic_cast< MessageReceiveBlock* >(b);
		std::vector<std::string> newBindingVars = receive -> getBindingVariable();
		parameterNames.insert(parameterNames.end(),newBindingVars.begin(),newBindingVars.end());
	}

	if ( not treeForLine.isLeaf( t ) ){
	
		std::vector< Token * > children = treeForLine.getChildren( t );

		for ( auto c = children.begin(); c < children.end(); c++ ){

			Block *newChild = tokenToBlock( *c, processName, parameterNames, globalVarNames );
			bt.addChild( b, newChild );
			secondParseProcessDef( *c, newChild, treeForLine, bt, processName, parameterNames, globalVarNames );
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
		Block *root = tokenToBlock( children[1], processName, pd.parameters, globalVars.getNames() );
		(pd.parseTree).setRoot( root );
		secondParseProcessDef( children[1], root, *pt, pd.parseTree, processName, pd.parameters, globalVars.getNames() );
		processName2Definition[ processName ] = pd;

#if DEBUG
std::cout << "---------------" << std::endl;
std::cout << "STARTING BLOCK PARSING ON :" << processName << std::endl;
std::cout << "Parse tree:" << std::endl;
printBlockTree( pd.parseTree, (pd.parseTree).getRoot() );
#endif
	}

	//check the processes that we've defined
	for ( auto pd = processName2Definition.begin(); pd != processName2Definition.end(); pd++ ){

		checkProcessDefinition( (pd -> second).parseTree.getRoot(), (pd -> second).parseTree, processName2Definition );
	}

	/*second round parse of system line */
	std::list< SystemProcess > system;
	secondParseSystemLine( tokenisedSystemLine, system, processName2Definition, globalVars );

#if defined DEBUG
exit(EXIT_SUCCESS);
#endif
	return std::make_pair(processName2Definition, system );
}
