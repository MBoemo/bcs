//----------------------------------------------------------
// Copyright 2017 University of Oxford
// Written by Michael A. Boemo (michael.boemo@path.ox.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <utility>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cassert>
#include "lexer.h"
#include "error_handling.h"

template <class T>
class Tree {

	private:
		T *_root;
		bool _rootSet = false;
		std::map< T *, std::vector< T * > > _children;
		std::map< T *, T * > _parents;
		std::vector< T * > _nodes;
		void buildSubtree( Tree<T> &newSubtree, T *node ){

			if ( not isLeaf(node) ){

				std::vector< T * > children = getChildren( node );
				for ( auto c = children.begin(); c < children.end(); c++ ){

					newSubtree.addChild( node, *c );
					buildSubtree( newSubtree, *c );
				}
			}
			else return;
		}

	public:
		void addChild( T *parent, T *newChild ){

			assert( std::find( _nodes.begin(), _nodes.end(), parent ) != _nodes.end() );
			assert( std::find( _nodes.begin(), _nodes.end(), newChild ) == _nodes.end() );
			assert( _rootSet );
			_children[ parent ].push_back( newChild );
			_parents[ newChild ] = parent;
			_nodes.push_back( newChild );
		}
		std::vector< T * > getChildren( T *node ){

			assert( _rootSet );
			return _children.at( node );
		}
		T *getParent( T *node ){

			assert( _rootSet );
			return _parents.at( node );
		}
		T *getRoot( void ){

			assert( _rootSet );
			return _root;
		}
		bool isRoot( T *node ){

			assert( _rootSet );
			bool areEqual = node == _root;
			return areEqual;
		}
		void setRoot( T *node ){

			_root = node;
			_nodes.push_back( node );
			_rootSet = true;
		}
		bool isLeaf( T *node ){

			assert( _rootSet );

			if ( std::find( _nodes.begin(), _nodes.end(), node ) != _nodes.end() and _children.find( node ) == _children.end() ){

				return true;
			}
			else{

				return false;
			}
		}
		std::vector< T * > getLeaves( void ){

			assert( _rootSet );

			std::vector< T * > leaves;
			for ( auto n = _nodes.begin(); n < _nodes.end(); n++ ){

				if ( _children.find( *n ) == _children.end() ){

					leaves.push_back( *n );
				}
			}
			return leaves;
		}
		inline void deleteNode( T *node ){

			assert( _rootSet );
			assert( std::find( _nodes.begin(), _nodes.end(), node ) != _nodes.end() );

			if ( _children.count( node ) > 0 ){

				std::vector< T * > children = _children.at( node );
				for ( auto c = children.begin(); c < children.end(); c++ ){

					deleteNode( *c );
				}

				if ( not isRoot(node) ){

					_children.at(_parents.at(node)).erase( std::find(_children.at(_parents.at(node)).begin(), _children.at(_parents.at(node)).end(), node ) );
				}

				_children.erase( _children.find( node ) );
				_parents.erase( _parents.find( node ) );
				_nodes.erase( std::find( _nodes.begin(), _nodes.end(), node ) );
			}
			else{

				if ( not isRoot(node) ){

					_children.at(_parents.at(node)).erase( std::find(_children.at(_parents.at(node)).begin(), _children.at(_parents.at(node)).end(), node ) );
				}

				_parents.erase( _parents.find( node ) );
				_nodes.erase( std::find( _nodes.begin(), _nodes.end(), node ) );
			}
		}
		Tree< T > getSubtree( T *node ){

			assert( std::find( _nodes.begin(), _nodes.end(), node ) != _nodes.end() );

			if ( node == _root ) return *this;

			Tree< T > newSubtree;
			newSubtree.setRoot( node );

			if ( isLeaf(node) ) return newSubtree;

			std::vector< T * > children = getChildren( node );
			
			for ( auto c = children.begin(); c < children.end(); c++ ){

				newSubtree.addChild( node, *c );
				buildSubtree( newSubtree, *c );
			}
			return newSubtree;
		}
		std::vector< T * > getNodes(void){ return _nodes; }
};


class GlobalVariables{

	public:
		std::map< std::string, int > intValues;
		std::map< std::string, double > doubleValues;
	
	GlobalVariables(){}
	GlobalVariables( const GlobalVariables &gv ){

		intValues = gv.intValues;
		doubleValues = gv.doubleValues;
	}
	void updateValue(std::string pName, double value){

		doubleValues[pName] = value;
	}
	void updateValue(std::string pName, int value){

		intValues[pName] = value;
	}
	void printValues(){
		std::cout << "int values:" << std::endl;
		for ( auto i = intValues.begin(); i != intValues.end(); i++) std::cout << i -> first << " " << i -> second << std::endl;
		std::cout << "double values:" << std::endl;
		for ( auto i = doubleValues.begin(); i != doubleValues.end(); i++) std::cout << i -> first << " " << i -> second << std::endl;
	}
};

/*function prototypes */
std::tuple< std::vector< Tree<Token> >, std::vector<Token *>, GlobalVariables > parseSource( std::vector< std::vector< Token * > > & );
void parseDefLine( Token *, std::vector< Token * > &, Tree<Token> & );
void printTree( Tree<Token>, Token * );//debugging

#endif
