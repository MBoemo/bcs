//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

//#define DEBUG 1

#include <string>
#include <tuple>
#include <utility>
#include <iostream>
#include "../lexer.h"
#include "../parser.h"
#include "../simulator.h"


static const char *help=
"bcs simulates a stochastic model written in the beacon calculus.\n"
"To run bcs, do:\n"
"  ./bcs [arguments] sourceCode.bc\n"
"Example:\n"
"  ./bcs -o simulationOutput sourceCode.bc\n"
"Required arguments are:\n"
"  -o,--output               output file name prefix,\n"
"  -s,--simulations          number of simulations to run.\n"
"Optional arguments are:\n"
"  -t,--threads              number of threads to use (default: 1),\n"
"  -m,--maxTrans             maximum number of transitions allowed per simulation (default: 1000000),\n"
"  -d,--maxDuration          maximum duration of each simulation(default: Inf),\n"
"  -h,--help                 show useage information.\n";


struct Arguments {

	std::string targetFilename;
	std::string outputFilename;
	int threads;
	int numOfSimulations;
	int maxTrans;
	double maxDuration;
};


Arguments parseArguments( int argc, char** argv ){

	if( argc < 2 ){

		std::cout << "Exiting with error.  No source code file specified." << std::endl << help << std::endl;
		exit(EXIT_FAILURE);
	}

	if ( std::string( argv[ 1 ] ) == "-h" or std::string( argv[ 1 ] ) == "--help" ){

		std::cout << help << std::endl;
		exit(EXIT_SUCCESS);
	}

	Arguments args;

	/*defaults - we'll override these if the option was specified by the user */
	args.outputFilename = "simulationOutput";
	args.threads = 1;
	args.numOfSimulations = 1;
	args.maxTrans = 1000000;
	args.maxDuration = std::numeric_limits<double>::max();

	/*parse the command line arguments */
	for ( int i = 1; i < argc; ){

		std::string flag( argv[ i ] );

		if ( flag == "-o" or flag == "--output" ){

			std::string strArg( argv[ i + 1 ] );
			args.outputFilename = strArg + ".simulation.bcs";
			i+=2;	
		}
		else if ( flag == "-s" or flag == "--simulations" ){

			std::string strArg( argv[ i + 1 ] );
			args.numOfSimulations = atoi( strArg.c_str() );
			i+=2;	
		}
		else if ( flag == "-m" or flag == "--maxTrans" ){

			std::string strArg( argv[ i + 1 ] );
			args.maxTrans = atoi( strArg.c_str() );
			i+=2;	
		}
		else if ( flag == "-d" or flag == "--maxDuration" ){

			std::string strArg( argv[ i + 1 ] );
			args.maxDuration = atof( strArg.c_str() );
			i+=2;	
		}
		else if ( flag == "-t" or flag == "--threads" ){

			std::string strArg( argv[ i + 1 ] );
			args.threads = atoi( strArg.c_str() );
			i+=2;	
		}
		/*NOTE: this is quite unsafe.  We could try to take a mistakenly input flag as the source code */
		else{

			if ( flag.substr(0,1) == "-" ){

				std::cout << "Exiting with error.  Unknown flag specified." << std::endl << help << std::endl;
				exit(EXIT_FAILURE);
			}

			args.targetFilename = flag;
			i+=1;
		}
	}

	return args;
}


int main( int argc, char** argv ){

	Arguments args = parseArguments( argc, argv );

	/*call lexer */
	std::vector< std::vector< Token * > > tokenisedSource = scanSource( args.targetFilename );

#if DEBUG
std::cout << "Finished lexer." << std::endl;
#endif

	/*call token parser */
	auto parsedSource = parseSource( tokenisedSource );

#if DEBUG
std::cout << "Finished parser." << std::endl;
#endif

	/*call block parser */
	auto blockParsed = secondPassParse( std::get<0>(parsedSource), std::get<1>(parsedSource), std::get<2>(parsedSource) );

#if DEBUG
std::cout << "Finished block parser." << std::endl;
#endif

	/*call the simulator */
	simulateSystem( blockParsed.first, blockParsed.second, std::get<2>(parsedSource), args.numOfSimulations, args.threads, args.outputFilename, args.maxTrans, args.maxDuration );

#if DEBUG
std::cout << "Finished simulation." << std::endl;
#endif

	return 0;
}
