// 
// Copyright (c) 2014 Toshiaki Takada
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
#ifndef _CLI_UTILS_HPP_
#define _CLI_UTILS_HPP_

#include <vector>
#include <string>

typedef map<string, string> ParamsMap;

typedef StringVector& (*CliParamsFilter)(StringVector&);
typedef map<string, CliParamsFilter> CliFilterFuncMap;

class CliUtils
{
public:
  CliUtils() { }

  void init();
  bool bind_if_interpreter(string& statement, TokenInputMap& input);

private:
  CliFilterFuncMap filter_map_;

  bool bind_if_get_token(string& str, string& token);
};

#endif /* _CLI_UTILS_HPP_ */
