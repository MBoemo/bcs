//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------


#include <string>
#include <tuple>
#include <utility>
#include <iostream>
#include "../lexer.h"
#include "../parser.h"
#include "../simulator.h"

static const char *test_help=
"bcs test executable.\n"
"To run bcs_test, do:\n"
"  ./bcs_test [arguments] sourceCode.bc\n"
"Example:\n"
"  ./bcs_test sourceCode.bc\n"
"Optional arguments are:\n"
"  --shouldFail              model passed is expected to fail instead of pass (default is pass).";


struct Arguments {

	std::string targetFilename;
	std::string outputFilename;
	int threads;
	int numOfSimulations;
	int maxTrans;
	double maxDuration;
	bool shouldFail;
};


Arguments parseTestArguments( int argc, char** argv ){

	if( argc < 2 ){

		std::cout << "Exiting with error.  No source code file specified." << std::endl << test_help << std::endl;
		exit(EXIT_FAILURE);
	}

	if ( std::string( argv[ 1 ] ) == "-h" or std::string( argv[ 1 ] ) == "--help" ){

		std::cout << test_help << std::endl;
		exit(EXIT_SUCCESS);
	}

	Arguments args;

	/*defaults - we'll override these if the option was specified by the user */
	args.outputFilename = "test.simulation.bcs";
	args.threads = 1;
	args.numOfSimulations = 1;
	args.maxTrans = 1000000;
	args.maxDuration = std::numeric_limits<double>::max();
	args.shouldFail = false;

	/*parse the command line arguments */
	for ( int i = 1; i < argc; ){

		std::string flag( argv[ i ] );

		if ( flag == "--shouldFail" ){

			args.shouldFail = true;
			i++;
		}
		else{

			if ( flag.substr(0,1) == "-" ){

				std::cout << "Exiting with error.  Unknown flag specified." << std::endl << test_help << std::endl;
				exit(EXIT_FAILURE);
			}

			args.targetFilename = flag;
			i+=1;
		}
	}

	return args;
}


int main( int argc, char** argv ){

	Arguments args = parseTestArguments( argc, argv );

	std::cout << "----------------------------------------------------------------------" << std::endl;
	std::cout << "TEST: " << args.targetFilename << std::endl;

	try{

		/*call lexer */
		std::vector< std::vector< Token * > > tokenisedSource = scanSource( args.targetFilename );

		/*call token parser */
		auto parsedSource = parseSource( tokenisedSource );

		/*call block parser */
		auto blockParsed = secondPassParse( std::get<0>(parsedSource), std::get<1>(parsedSource), std::get<2>(parsedSource) );

		/*call the simulator */
		simulateSystem( blockParsed.first, blockParsed.second, std::get<2>(parsedSource), args.numOfSimulations, args.threads, args.outputFilename, args.maxTrans, args.maxDuration );

		if (not args.shouldFail) std::cout << "PASS" << std::endl;
		else std::cout << "FAIL" << std::endl;
	}
	catch(...){

		if (not args.shouldFail) std::cout << "FAIL" << std::endl;
		else std::cout << "PASS" << std::endl;	
	}
	return 0;
}
