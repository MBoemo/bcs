#include "../../src/numerical.h"
#include <iostream>

int main(void){

	double j = 5.1;
	double k = 7.1;

	unsigned_numerical u1, u2, u3;
	u1 = j;
	u2 = k;

	bool test_bool = u1 < u2;
	std::cout << test_bool << std::endl;
	std::cout << u1.return_floatingPointValue() << std::endl;

	u3 = u1 + u2;
	std::cout << u3.return_floatingPointValue() << std::endl;

	u3 = u1 * u2;
	std::cout << u3.return_floatingPointValue() << std::endl;


	u3 = u1 / u2;
	std::cout << u3.return_floatingPointValue() << std::endl;

	std::string string1 = "1.2";
	u3 = string1;
	std::cout << u3.return_floatingPointValue() << std::endl;

	std::string string2 = "00000000005.60000000000000";
	u3 = string2;
	std::cout << u3.return_floatingPointValue() << std::endl;

	//////////////////////////////////////////////////////////

	signed_numerical s1, s2, s3;
	s1 = -1.*j;
	s2 = -1.*k;
	std::cout << s1.return_floatingPointValue() << std::endl;
	std::cout << s2.return_floatingPointValue() << std::endl;

	s3 = s1 + s2;
	std::cout << s3.return_floatingPointValue() << std::endl;

	s3 = s1 - s2;
	std::cout << s3.return_floatingPointValue() << std::endl;

	s3 = s1 * s2;
	std::cout << s3.return_floatingPointValue() << std::endl;

	s3 = s1 / s2;
	std::cout << s3.return_floatingPointValue() << std::endl;

	string1 = "-.2";
	s3 = string1;
	std::cout << s3.return_floatingPointValue() << std::endl;

	std::cout << s1.return_floatingPointValue() << std::endl;
	s1.inplace_negation();
	std::cout << s1.return_floatingPointValue() << std::endl;

	std::cout << s2.return_floatingPointValue() << std::endl;
	s2.inplace_absVal();
	std::cout << s2.return_floatingPointValue() << std::endl;

	string1 = "100000000";
	s3 = string1;
	std::cout << s3.return_floatingPointValue() << std::endl;

	string1 = "0.000000001";
	s3 = string1;
	std::cout << s3.return_floatingPointValue() << std::endl;

	//check underflow
	string1 = "0.0000000001";
	s3 = string1;
	std::cout << s3.return_floatingPointValue() << std::endl;

	return 0;
}
