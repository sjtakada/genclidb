#ifndef _PROJECT_HPP_
#define _PROJECT_HPP_

// C headers.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// C++ headers.
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

// Generic string vector.
typedef vector<string> StringVector;

// e.g., "IPV4-ADDR:1.0" => "1.2.3.4" -- CLI def token/param to input.
typedef map<string, string> ParamsMap;

#endif /* _PROJECT_HPP_ */
