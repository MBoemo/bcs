//----------------------------------------------------------
// Copyright 2017-2020 University of Oxford
// Written by Michael A. Boemo (mb915@cam.ac.uk)
// This software is licensed under GPL-2.0.  You should have
// received a copy of the license with this software.  If
// not, please Email the author.
//----------------------------------------------------------

#ifndef NUMERICAL_H
#define NUMERICAL_H

#include <limits>
#include <cassert>
#include <string>
#include <iostream>
#include <math.h>

#define NUM_INT_BITS 32
#define NUM_FRAC_BITS 32


class signed_numerical{

	private:
		int64_t FIXEDPOINT_ONE = (1l << NUM_FRAC_BITS);
		int64_t FIXEDPOINT_ZERO = (0l << NUM_FRAC_BITS);
		int64_t value_fixedPoint = FIXEDPOINT_ZERO;
		bool isFloat = false;
		int64_t sign = 1l;
		int64_t MASK_FRAC_BITS = FIXEDPOINT_ONE - 1l;

		void warnUnderflow(std::string s){

			std::cerr << "Warning: Overflow/underflow on signed_numerical from std::string constructor.\n";
			std::cerr << "  Value is: " << s << std::endl;
		}
		void warnUnderflow(const int &i){

			std::cerr << "Warning: Overflow/underflow on signed_numerical from int constructor.\n";
			std::cerr << "  Value is: " << std::to_string(i) << std::endl;
		}
		void warnUnderflow(const double &i){

			std::cerr << "Warning: Overflow/underflow on signed_numerical from double constructor.\n";
			std::cerr << "  Value is: " << std::to_string(i) << std::endl;
		}

	public:
		int64_t return_fixedPointValue(void) const{

			return value_fixedPoint;
		}

		long double return_floatingPointValue(void) const{

			return ((long double)value_fixedPoint / (long double)FIXEDPOINT_ONE);
		}
		int64_t return_sign(void) const{

			return sign;
		}
		bool return_isFloat(void) const{

			return isFloat;
		}
		void inplace_negation(void){

			value_fixedPoint *= -1;
		}
		void inplace_absVal(void){

			if ( value_fixedPoint > FIXEDPOINT_ZERO ){
				return;
			}
			else{
				value_fixedPoint = FIXEDPOINT_ZERO - value_fixedPoint;
			}
		}
		signed_numerical& operator+=(const signed_numerical& rhs){
			value_fixedPoint += rhs.return_fixedPointValue();
			return *this;
		}
		signed_numerical& operator-=(const signed_numerical& rhs){
			value_fixedPoint -= rhs.return_fixedPointValue();
			return *this;
		}
		signed_numerical& operator*=(const signed_numerical& rhs){

			int64_t intPart_lhs = value_fixedPoint >> NUM_FRAC_BITS;
			int64_t intPart_rhs = rhs.return_fixedPointValue() >> NUM_FRAC_BITS;

			int64_t floatPart_lhs = value_fixedPoint & MASK_FRAC_BITS;
			int64_t floatPart_rhs = rhs.return_fixedPointValue() & MASK_FRAC_BITS;

			int64_t result = 0;
		    result += (intPart_lhs * intPart_rhs) << NUM_FRAC_BITS;
		    result += (intPart_lhs * floatPart_rhs);
		    result += (floatPart_lhs * intPart_rhs);
		    result += ((floatPart_lhs * floatPart_rhs) >> NUM_FRAC_BITS) & MASK_FRAC_BITS;
		    value_fixedPoint = result;

			return *this;
		}
		signed_numerical& operator/=(const signed_numerical& rhs){

		    int64_t reciprocal = 1l;
		    reciprocal <<= 2*NUM_FRAC_BITS - 1;
		    reciprocal = reciprocal / rhs.return_fixedPointValue();
		    value_fixedPoint = (value_fixedPoint * reciprocal) >> NUM_FRAC_BITS;
		    value_fixedPoint <<= 1;

			return *this;
		}
		signed_numerical& operator=( const int &i){

			value_fixedPoint = ((int64_t)i << NUM_FRAC_BITS);
			return *this;
		}
		signed_numerical& operator=( const double &i){

			int64_t intPart = (int) i;
			long double floatPart = i - intPart;
			if (floatPart > 0.) isFloat = true;
			intPart = intPart << NUM_FRAC_BITS;
			value_fixedPoint = intPart + (int64_t)(FIXEDPOINT_ONE * floatPart);
			return *this;
		}
		signed_numerical& operator=( const std::string &s ){

			std::string numericalString = s;

			bool isNegative = false;
			if (numericalString.substr(0,1) == "-"){
				isNegative = true;
				numericalString = numericalString.substr(1);
			}

			//trim
			for (size_t i = 0; i < s.length(); i++){

				if (numericalString.substr(0,1) == "0") numericalString = numericalString.substr(1);
			}
			char &lastChar = numericalString.back();
			while (lastChar == '0'){
				numericalString.pop_back();
				lastChar = numericalString.back();
			}

			size_t i = numericalString.find(".");
			if (i != std::string::npos){

				isFloat = true;

				int64_t intPart = 0;
				std::string intPartSubstr = numericalString.substr(0,i);
				if (intPartSubstr.length() > 9) warnUnderflow(numericalString);
				if (intPartSubstr.size() > 0){
					intPart = std::stoi(numericalString.substr(0,i));
				}
				intPart = intPart << NUM_FRAC_BITS;
				value_fixedPoint = (long int) intPart;

				std::string floatPartSubstr = numericalString.substr(i+1);
				int64_t numDigits = floatPartSubstr.size();

				if (numDigits > 9) warnUnderflow(numericalString);

				long double floatPart_d = 0;
				int64_t floatPart = 0;
				if (numDigits > 0){
					floatPart_d = std::stod(floatPartSubstr);
					floatPart_d = floatPart_d * pow(10., -1.*numDigits);
					floatPart = floatPart_d*FIXEDPOINT_ONE;
					if (intPart < 0){
						value_fixedPoint -= (int64_t)FIXEDPOINT_ONE;
						floatPart = FIXEDPOINT_ONE - floatPart;
					}
				}

				value_fixedPoint |= (long int)(floatPart & MASK_FRAC_BITS);
				if (isNegative) value_fixedPoint *= -1;
			}
			else{
				int64_t intPart = std::stoi(s);
				value_fixedPoint = (long int) intPart << NUM_FRAC_BITS;
			}

			return *this;
		}
		friend signed_numerical operator+ (signed_numerical lhs, const signed_numerical & rhs){
			lhs += rhs;
			return lhs;
		}
		friend signed_numerical operator- (signed_numerical lhs, const signed_numerical & rhs){
			lhs -= rhs;
			return lhs;
		}
		friend signed_numerical operator* (signed_numerical lhs, const signed_numerical & rhs){
			lhs *= rhs;
			return lhs;
		}
		friend signed_numerical operator/ (signed_numerical lhs, const signed_numerical & rhs){
			lhs /= rhs;
			return lhs;
		}
		bool operator==(const signed_numerical & rhs) const{
			return value_fixedPoint == rhs.return_fixedPointValue();
		}
		bool operator!=(const signed_numerical & rhs) const{
			return value_fixedPoint  != rhs.return_fixedPointValue();
		}
		bool operator<(const signed_numerical & rhs) const{
			return value_fixedPoint < rhs.return_fixedPointValue();
		}
		bool operator<=(const signed_numerical & rhs) const{
			return value_fixedPoint <= rhs.return_fixedPointValue();
		}
		bool operator>(const signed_numerical & rhs) const{
			return value_fixedPoint  > rhs.return_fixedPointValue();
		}
		bool operator>=(const signed_numerical & rhs) const{
			return value_fixedPoint  >= rhs.return_fixedPointValue();
		}
};


class unsigned_numerical{

	private:
		uint64_t FIXEDPOINT_ONE = (1l << NUM_FRAC_BITS);
		uint64_t FIXEDPOINT_ZERO = (0l << NUM_FRAC_BITS);
		uint64_t value_fixedPoint = FIXEDPOINT_ZERO;
		bool isFloat = false;
		uint64_t MASK_FRAC_BITS = FIXEDPOINT_ONE - 1l;

		void warnUnderflow(std::string s){

			std::cerr << "Warning: Overflow/underflow on unsigned_numerical from std::string constructor.\n";
			std::cerr << "  Value is: " << s << std::endl;
		}
		void warnUnderflow(const int &i){

			std::cerr << "Warning: Overflow/underflow on unsigned_numerical from int constructor.\n";
			std::cerr << "  Value is: " << std::to_string(i) << std::endl;
		}
		void warnUnderflow(const double &i){

			std::cerr << "Warning: Overflow/underflow on unsigned_numerical from double constructor.\n";
			std::cerr << "  Value is: " << std::to_string(i) << std::endl;
		}

	public:
		uint64_t return_fixedPointValue(void) const{

			return value_fixedPoint;
		}

		long double return_floatingPointValue(void) const{

			return ((long double)value_fixedPoint / (long double)FIXEDPOINT_ONE);
		}

		bool return_isFloat(void){

			return isFloat;
		}
		unsigned_numerical& operator+=(const unsigned_numerical& rhs){
			value_fixedPoint += rhs.return_fixedPointValue();
			return *this;
		}
		unsigned_numerical& operator-=(const unsigned_numerical& rhs){
			value_fixedPoint -= rhs.return_fixedPointValue();
			return *this;
		}
		unsigned_numerical& operator*=(const unsigned_numerical& rhs){

			uint64_t intPart_lhs = value_fixedPoint >> NUM_FRAC_BITS;
			uint64_t intPart_rhs = rhs.return_fixedPointValue() >> NUM_FRAC_BITS;

			uint64_t floatPart_lhs = value_fixedPoint & MASK_FRAC_BITS;
			uint64_t floatPart_rhs = rhs.return_fixedPointValue() & MASK_FRAC_BITS;

			uint64_t result = 0;
		    result += (intPart_lhs * intPart_rhs) << NUM_FRAC_BITS;
		    result += (intPart_lhs * floatPart_rhs);
		    result += (floatPart_lhs * intPart_rhs);
		    result += ((floatPart_lhs * floatPart_rhs) >> NUM_FRAC_BITS) & MASK_FRAC_BITS;
		    value_fixedPoint = result;

			return *this;
		}
		unsigned_numerical& operator/=(const unsigned_numerical& rhs){

		    uint64_t reciprocal = 1l;
		    reciprocal <<= 2*NUM_FRAC_BITS - 1;
		    reciprocal = reciprocal / rhs.return_fixedPointValue();
		    value_fixedPoint = (value_fixedPoint * reciprocal) >> NUM_FRAC_BITS;
		    value_fixedPoint <<= 1;

			return *this;
		}
		unsigned_numerical& operator=(const signed_numerical &i){

			assert( i.return_sign() > 0);
			value_fixedPoint = i.return_fixedPointValue();
			isFloat = i.return_isFloat();
			return *this;
		}
		unsigned_numerical& operator=( const unsigned int &i){

			value_fixedPoint = ((uint64_t)i << NUM_FRAC_BITS);
			return *this;
		}
		unsigned_numerical& operator=( const double &i){

			assert(i >= 0.);

			uint64_t intPart = (uint64_t) i;
			long double floatPart = i - intPart;
			if (floatPart > 0.) isFloat = true;
			intPart = intPart << NUM_FRAC_BITS;
			value_fixedPoint = intPart + (uint64_t)(FIXEDPOINT_ONE * floatPart);
			return *this;
		}
		unsigned_numerical& operator=( const std::string &s ){

			std::string numericalString = s;

			//trim
			for (size_t i = 0; i < s.length(); i++){

				if (numericalString.substr(0,1) == "0") numericalString = numericalString.substr(1);
			}
			char &lastChar = numericalString.back();
			while (lastChar == '0'){
				numericalString.pop_back();
				lastChar = numericalString.back();
			}

			size_t i = numericalString.find(".");
			if (i != std::string::npos){

				isFloat = true;

				uint64_t intPart = 0;
				std::string intPartSubstr = numericalString.substr(0,i);
				if (intPartSubstr.length() > 9) warnUnderflow(numericalString);
				if (intPartSubstr.size() > 0){
					intPart = std::stoi(numericalString.substr(0,i));
				}
				intPart = intPart << NUM_FRAC_BITS;
				value_fixedPoint = intPart;

				std::string floatPartSubstr = numericalString.substr(i+1);
				uint64_t numDigits = floatPartSubstr.size();
				if (numDigits > 9) warnUnderflow(numericalString);

				long double floatPart_d = 0;
				uint64_t floatPart = 0;
				if (numDigits > 0){
					floatPart_d = std::stod(floatPartSubstr);
					floatPart_d = floatPart_d * pow(10., -1.*numDigits);
					floatPart = floatPart_d*FIXEDPOINT_ONE;
					if (intPart < 0){
						value_fixedPoint -= (uint64_t)FIXEDPOINT_ONE;
						floatPart = FIXEDPOINT_ONE - floatPart;
					}
				}
				value_fixedPoint |= (floatPart & MASK_FRAC_BITS);
			}
			else{
				uint64_t intPart = std::stoll(numericalString);
				value_fixedPoint = intPart << NUM_FRAC_BITS;
			}
			return *this;
		}
		friend unsigned_numerical operator+ (unsigned_numerical lhs, const unsigned_numerical & rhs){
			lhs += rhs;
			return lhs;
		}
		friend unsigned_numerical operator- (unsigned_numerical lhs, const unsigned_numerical & rhs){
			lhs -= rhs;
			return lhs;
		}
		friend unsigned_numerical operator* (unsigned_numerical lhs, const unsigned_numerical & rhs){
			lhs *= rhs;
			return lhs;
		}
		friend unsigned_numerical operator/ (unsigned_numerical lhs, const unsigned_numerical & rhs){
			lhs /= rhs;
			return lhs;
		}
		bool operator==(const unsigned_numerical & rhs) const{
			return value_fixedPoint == rhs.return_fixedPointValue();
		}
		bool operator!=(const unsigned_numerical & rhs) const{
			return value_fixedPoint  != rhs.return_fixedPointValue();
		}
		bool operator<(const unsigned_numerical & rhs) const{
			return value_fixedPoint < rhs.return_fixedPointValue();
		}
		bool operator<=(const unsigned_numerical & rhs) const{
			return value_fixedPoint <= rhs.return_fixedPointValue();
		}
		bool operator>(const unsigned_numerical & rhs) const{
			return value_fixedPoint  > rhs.return_fixedPointValue();
		}
		bool operator>=(const unsigned_numerical & rhs) const{
			return value_fixedPoint  >= rhs.return_fixedPointValue();
		}
};


#endif
