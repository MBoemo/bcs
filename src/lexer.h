//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <tuple>
#include <set>

class Token{
	
	protected:
		std::string _raw, _identity;
		unsigned int _lineNumber, _column;

	public:
		std::string identify( void ) { return _identity; }
		std::string value( void ) { return _raw; }
		unsigned int getLine( void ) { return _lineNumber; }
		unsigned int getColumn( void ) { return _column; }
		Token( std::string identity, std::string raw, unsigned int lineNumber, unsigned int column ){

			_identity = identity;
			_raw = raw;
			_lineNumber = lineNumber;
			_column = column;
		}
};

class FiniteStateAutomaton {

	public:
		void add_edge( std::string, std::set< char >, std::string );
		void designate_endState( std::string );
		std::string testString( std::string );
		std::string startState = "startState";

	private:
		std::vector< std::tuple< std::string, std::set< char >, std::string > > edges;
		std::vector< std::string > endStates;
};

std::vector< Token * > scanLine( std::string &, unsigned int, unsigned int );
std::vector< std::vector< Token * > > scanSource( std::string & );

#endif
