//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef BLOCKPARSER_H
#define BLOCKPARSER_H

#include <vector>
#include <list>
#include <cassert>
#include <string>
#include <tuple>
#include <iostream>
#include "parser.h"
#include "lexer.h"

class Block{

	protected:
		Token * inputToken;
		Block( Token * t, std::string &name ){inputToken = t;}

	public:
		virtual Token * getToken(void) const = 0;
		virtual std::string identify( void ) const = 0;
		virtual std::vector< Token * > getRate( void ) const = 0;
		virtual std::string getOwningProcess( void ) const = 0;
};

class ActionBlock: public Block {

	private:
		std::string _owningProcess;
		Token *_underlyingToken;
		std::vector< Token * > _RPNrate;

	public:
		ActionBlock( Token *, std::string );
		ActionBlock( const ActionBlock &ab ) : Block(ab){

			actionName = ab.actionName;
			_RPNrate = ab.getRate();
		}
		Token * getToken(void) const {return _underlyingToken;}
		std::string identify( void ) const { return "Action"; }
		std::string getOwningProcess( void ) const { return _owningProcess; }
		std::string actionName;
		std::vector< Token * > getRate( void ) const { return _RPNrate; }
};

class ChoiceBlock: public Block {

	private:
		std::string _owningProcess;
		Token *_underlyingToken;

	public:
		Token * getToken(void) const {return _underlyingToken;}
		ChoiceBlock( Token *, std::string );
		ChoiceBlock( const ChoiceBlock &cb ) : Block(cb) {}
		std::string identify( void ) const { return "Choice"; }
		std::vector< Token * > getRate( void ) const { assert( false ); }
		std::string getOwningProcess( void ) const { return _owningProcess; }
};

class ParallelBlock: public Block {

	private:
		std::string _owningProcess;
		Token *_underlyingToken;

	public:
		Token * getToken(void) const {return _underlyingToken;}
		ParallelBlock( Token *, std::string );
		ParallelBlock( const ParallelBlock &cb ) : Block(cb) {}
		std::string identify( void ) const { return "Parallel"; }
		std::vector< Token * > getRate( void ) const { assert( false ); }
		std::string getOwningProcess( void ) const { return _owningProcess; }
};

class GateBlock: public Block {

	protected:
		std::string _owningProcess;
		Token *_underlyingToken;
		std::vector< Token * > _RPNexpression;
	public:
		Token * getToken(void) const {return _underlyingToken;}
		GateBlock( Token *, std::string );
		GateBlock( const GateBlock &gb ) : Block(gb){

			_RPNexpression = gb.getConditionExpression();
		}
		std::string identify( void ) const { return "Gate"; }
		std::vector< Token * > getRate( void ) const { assert( false ); }
		std::string getOwningProcess( void ) const { return _owningProcess; }
		std::vector< Token * > getConditionExpression( void ) const { return _RPNexpression; }
};

class MessageReceiveBlock: public Block {

	protected:
		bool _handshake, _check, _usesSets=false,_hasBindingVar=false;
		std::string _owningProcess;
		Token *_underlyingToken;
		std::vector< std::vector< Token * > > _channelNames;
		std::vector< std::string > _bindingVariables;
		std::vector< std::vector< Token * > > _RPNexpressions;
		std::vector< Token * > _RPNrate;

	public:
		MessageReceiveBlock( Token *, std::string );
		MessageReceiveBlock( const MessageReceiveBlock &mb ) : Block(mb){

			_handshake = mb.isHandshake();
			_check = mb.isCheck();
			_hasBindingVar = mb.bindsVariable();
			_RPNexpressions = mb.getSetExpression();
			_channelNames = mb.getChannelName();
			_bindingVariables = mb.getBindingVariable();
			_RPNrate = mb.getRate();
		}
		Token * getToken(void) const {return _underlyingToken;}
		bool isHandshake( void ) const { return _handshake; }
		bool isCheck( void ) const { return _check; }
		bool bindsVariable( void ) const { return _hasBindingVar; }
		bool usesSets( void ) const { return _usesSets; }
		std::vector< std::vector< Token * > > getChannelName( void ) const { return _channelNames; }
		std::vector< std::string > getBindingVariable( void ) const { return _bindingVariables; }
		std::vector< std::vector< Token * > > getSetExpression( void ) const { return _RPNexpressions; }
		std::string identify( void ) const { return "MessageReceive"; }
		std::string getOwningProcess( void ) const { return _owningProcess; }
		std::vector< Token * > getRate( void ) const { return _RPNrate; }
};

class MessageSendBlock: public Block {

	protected:
		bool _handshake, _kill;
		std::string _owningProcess;
		Token *_underlyingToken;
		std::vector< std::vector< Token * > > _channelNames;
		std::vector< std::vector< Token * > > _RPNexpressions;
		std::vector< Token * > _RPNrate;
	public:
		MessageSendBlock( Token *, std::string );
		MessageSendBlock( const MessageSendBlock &mb ) : Block(mb){

			_handshake = mb.isHandshake();
			_kill = mb.isKill();
			_RPNexpressions = mb.getParameterExpression();
			_channelNames = mb.getChannelName();
			_RPNrate = mb.getRate();
		}
		Token * getToken(void) const {return _underlyingToken;}
		bool isHandshake( void ) const { return _handshake; }
		bool isKill( void ) const { return _kill; }
		std::vector< std::vector< Token * > > getChannelName( void ) const { return _channelNames; }
		std::vector< std::vector< Token * > > getParameterExpression( void ) const { return _RPNexpressions; }
		std::string identify( void ) const { return "MessageSend"; }
		std::string getOwningProcess( void ) const { return _owningProcess; }
		std::vector< Token * > getRate( void ) const { return _RPNrate; }
};

class ProcessBlock: public Block {

	protected:
		std::string _processName, _owningProcess;
		std::vector< std::vector<Token * > > _parameterExpressions;
		Token *_underlyingToken;
	public:
		ProcessBlock( Token *, std::string );
		ProcessBlock( const ProcessBlock &pb ) : Block(pb) {

			_processName = pb.getProcessName();
			_parameterExpressions = pb.getParameterExpressions();
		}
		Token * getToken(void) const {return _underlyingToken;}
		std::string identify( void ) const { return "Process"; }
		std::string getProcessName( void ) const { return _processName; }
		std::vector< std::vector< Token * > > getParameterExpressions( void ) const { return _parameterExpressions; }
		std::vector< Token * > getRate( void ) const { assert( false ); }
		std::string getOwningProcess( void ) const { return _owningProcess; }
};


class ProcessDefinition{

	public:
		Tree<Block> parseTree;
		std::vector< std::string > parameters;
};

class SystemProcess;
class Candidate;



class ParameterValues{

	public:
		std::map< std::string, Numerical > values;
	
	ParameterValues(){}
	ParameterValues( const ParameterValues &pv ){

		values = pv.values;
	}
	void updateValue(std::string pName, Numerical value){

		if ( values.count(pName) > 0 ) values.erase(values.find(pName));
		values[pName] = value;
	}
	void printValues(){
		std::cout << "number of values: " << values.size() << std::endl;
		for ( auto i = values.begin(); i != values.end(); i++){

			if ((i->second).isDouble()){

				std::cout << i -> first << " " << (i -> second).getDouble() << " Double" << std::endl;
			}
			else{

				std::cout << i -> first << " " << (i -> second).getInt() << " Int" << std::endl;
			}
		}
	}
};

class SystemProcess{

	public:
		Tree<Block> parseTree;
		ParameterValues parameterValues;
		std::map< std::string, Numerical > localVariables; //system line variable substitutions and bound variables
		SystemProcess(){}
		SystemProcess( const SystemProcess &sp ){

			parseTree = sp.parseTree;
			parameterValues = sp.parameterValues;
			localVariables = sp.localVariables;
		}
};


class Candidate{

	public:
		Block *actionCandidate;
		ParameterValues parameterValues;
		std::map< std::string, Numerical > localVariables;
		SystemProcess *processInSystem;
		double rate;
		std::vector< Numerical > rangeEvaluation;
		std::list< SystemProcess > parallelProcesses;
		Candidate( Block *b, ParameterValues pv, std::map< std::string, Numerical > lv, SystemProcess *si, std::list< SystemProcess > pp ){

			actionCandidate = b;
			parameterValues = pv;
			localVariables = lv;
			processInSystem = si;
			parallelProcesses = pp;
		}
		std::vector< std::vector< Token * > > getChannelName(void){
	
			assert( actionCandidate -> identify() == "MessageSend" or actionCandidate -> identify() == "MessageReceive" );
			std::vector< std::vector< Token * > > channelName;
			if ( actionCandidate -> identify() == "MessageSend" ){

				MessageSendBlock *msb = dynamic_cast< MessageSendBlock * >(actionCandidate);
				channelName = msb -> getChannelName();
			}
			else if (actionCandidate -> identify() == "MessageReceive"){

				MessageReceiveBlock *mrb = dynamic_cast< MessageReceiveBlock * >(actionCandidate);
				channelName = mrb -> getChannelName();
			}
			return channelName; 
		}
};


/*function prototypes */
std::pair< std::map< std::string, ProcessDefinition >, std::list< SystemProcess > > secondPassParse( std::vector< Tree<Token> >, std::vector< Token* >, GlobalVariables & );
void printBlockTree( Tree<Block>, Block * );

#endif
