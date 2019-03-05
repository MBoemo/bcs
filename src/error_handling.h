//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <exception>
#include <string.h>
#include <string> 

struct BadSourcePath : public std::exception {
	const char * what () const throw () {
		return "Could not open source file.";
	}
};

struct BadOutputPath : public std::exception {
	const char * what () const throw () {
		return "Could not open output target file.";
	}
};

struct UnbalancedParentheses : public std::exception {
	std::string badToken, lineNum, colNum;	
	UnbalancedParentheses( Token *t ){

		badToken = t -> value();
		lineNum = std::to_string( t -> getLine() );
		colNum = std::to_string( t -> getColumn() );
	}
	const char* what () const throw () {
		const char* genericLine = "Line ";
		const char* genericCol = " Column ";
		const char* message = ". Unbalanced parentheses thrown on token: ";
		const char* cc_badToken = badToken.c_str();
		const char* cc_lineNum = lineNum.c_str();
		const char* cc_colNum = colNum.c_str();
		size_t msg_length = strlen(cc_badToken) + strlen(cc_lineNum) + strlen(cc_colNum) + 100;
		char* error_msg = static_cast<char*>(calloc(msg_length, sizeof(char)));
		strcpy( error_msg, genericLine );
		strcat( error_msg, cc_lineNum );
		strcat( error_msg, genericCol );
		strcat( error_msg, cc_colNum );
		strcat( error_msg, message );
		strcat( error_msg, cc_badToken );

		return error_msg;
	}	
};

struct NoMachinePath : public std::exception {
	std::string badToken, lineNum, colNum;	
	NoMachinePath( std::string v, unsigned int line, unsigned int col ){

		badToken = v;
		lineNum = std::to_string( line );
		colNum = std::to_string( col );
	}
	const char* what () const throw () {
		const char* genericLine = "Line ";
		const char* genericCol = " Column ";
		const char* message = ". No valid path found in tokenisation automaton.  Syntax error on: ";
		const char* cc_badToken = badToken.c_str();
		const char* cc_lineNum = lineNum.c_str();
		const char* cc_colNum = colNum.c_str();
		size_t msg_length = strlen(cc_badToken) + strlen(cc_lineNum) + strlen(cc_colNum) + 100;
		char* error_msg = static_cast<char*>(calloc(msg_length, sizeof(char)));
		strcpy( error_msg, genericLine );
		strcat( error_msg, cc_lineNum );
		strcat( error_msg, genericCol );
		strcat( error_msg, cc_colNum );
		strcat( error_msg, message );
		strcat( error_msg, cc_badToken );

		return error_msg;
	}	
};

struct SyntaxError : public std::exception {
	std::string badToken, lineNum, colNum, specifics;	
	SyntaxError( Token *t, std::string s ){

		badToken = t -> value();
		lineNum = std::to_string( t -> getLine() );
		colNum = std::to_string( t -> getColumn() );
		specifics = "\n" + s;
	}
	const char* what () const throw () {
		const char* genericLine = "Line ";
		const char* genericCol = " Column ";
		const char* message = ". Syntax error thrown on token: ";
		const char* cc_badToken = badToken.c_str();
		const char* cc_lineNum = lineNum.c_str();
		const char* cc_colNum = colNum.c_str();
		const char* cc_specifics = specifics.c_str();
		size_t msg_length = strlen(cc_badToken) + strlen(cc_lineNum) + strlen(cc_colNum) + 1000;
		char* error_msg = static_cast<char*>(calloc(msg_length, sizeof(char)));
		strcpy( error_msg, genericLine );
		strcat( error_msg, cc_lineNum );
		strcat( error_msg, genericCol );
		strcat( error_msg, cc_colNum );
		strcat( error_msg, message );
		strcat( error_msg, cc_badToken );
		strcat( error_msg, cc_specifics );

		return error_msg;
	}	
};

struct UndefinedVariable : public std::exception {
	std::string badToken, lineNum, colNum;	
	UndefinedVariable( Token *t ){

		badToken = t -> value();
		lineNum = std::to_string( t -> getLine() );
		colNum = std::to_string( t -> getColumn() );
	}
	const char* what () const throw () {
		const char* genericLine = "Line ";
		const char* genericCol = " Column ";
		const char* message = ". Variable or process not defined, thrown on token: ";
		const char* cc_badToken = badToken.c_str();
		const char* cc_lineNum = lineNum.c_str();
		const char* cc_colNum = colNum.c_str();
		size_t msg_length = strlen(cc_badToken) + strlen(cc_lineNum) + strlen(cc_colNum) + 100;
		char* error_msg = static_cast<char*>(calloc(msg_length, sizeof(char)));
		strcpy( error_msg, genericLine );
		strcat( error_msg, cc_lineNum );
		strcat( error_msg, genericCol );
		strcat( error_msg, cc_colNum );
		strcat( error_msg, message );
		strcat( error_msg, cc_badToken );

		return error_msg;
	}	
};

struct BadRate : public std::exception {
	std::string badToken, lineNum, colNum;	
	BadRate( Token *t ){

		badToken = t -> value();
		lineNum = std::to_string( t -> getLine() );
		colNum = std::to_string( t -> getColumn() );
	}
	const char* what () const throw () {
		const char* genericLine = "Line ";
		const char* genericCol = " Column ";
		const char* message = ". Rate must be greater than 0, thrown on token: ";
		const char* cc_badToken = badToken.c_str();
		const char* cc_lineNum = lineNum.c_str();
		const char* cc_colNum = colNum.c_str();
		size_t msg_length = strlen(cc_badToken) + strlen(cc_lineNum) + strlen(cc_colNum) + 100;
		char* error_msg = static_cast<char*>(calloc(msg_length, sizeof(char)));
		strcpy( error_msg, genericLine );
		strcat( error_msg, cc_lineNum );
		strcat( error_msg, genericCol );
		strcat( error_msg, cc_colNum );
		strcat( error_msg, message );
		strcat( error_msg, cc_badToken );

		return error_msg;
	}	
};

struct WrongType : public std::exception {
	std::string badToken, lineNum, colNum, specifics;	
	WrongType( Token *t, std::string s ){

		badToken = t -> value();
		lineNum = std::to_string( t -> getLine() );
		colNum = std::to_string( t -> getColumn() );
		specifics = "\n" + s;
	}
	const char* what () const throw () {
		const char* genericLine = "Line ";
		const char* genericCol = " Column ";
		const char* message = ". Operand is of wrong type for operator given by token: ";
		const char* cc_badToken = badToken.c_str();
		const char* cc_lineNum = lineNum.c_str();
		const char* cc_colNum = colNum.c_str();
		const char* cc_specifics = specifics.c_str();
		size_t msg_length = strlen(cc_badToken) + strlen(cc_lineNum) + strlen(cc_colNum) + 1000;
		char* error_msg = static_cast<char*>(calloc(msg_length, sizeof(char)));
		strcpy( error_msg, genericLine );
		strcat( error_msg, cc_lineNum );
		strcat( error_msg, genericCol );
		strcat( error_msg, cc_colNum );
		strcat( error_msg, message );
		strcat( error_msg, cc_badToken );
		strcat( error_msg, cc_specifics );

		return error_msg;
	}	
};


struct MultipleSystemLines : public std::exception {
	const char * what () const throw () {
		return "Source has multiple system lines.  Only one is allowed.";
	}
};

struct SystemLineMissing : public std::exception {
	const char * what () const throw () {
		return "No system in line in source file.";
	}
};

#endif
