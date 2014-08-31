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

// Base functor
class CliFunctor
{
public:
  virtual StringVector& operator() (StringVector& vec) const = 0;
};

typedef map<string, CliFunctor *> CliFunctorMap;


// Area ID and Format Functor.
class CliFunctorAreaIDandFormat: public CliFunctor
{
public:
  StringVector& operator() (StringVector& vec) const;
};

// IPV4Prefix to Address Masklen Functor.
class CliFunctorIPv4Prefix2AddressMasklen: public CliFunctor
{
public:
  StringVector& operator() (StringVector& vec) const;
};

// Apply Mask IPv4 Functor.
class CliFunctorApplyMaskIPv4: public CliFunctor
{
public:
  StringVector& operator() (StringVector& vec) const;
};

class CliUtils
{
public:
  CliUtils() { }

  void init();
  bool bind_interpreter(string& statement, ParamsMap& input);

private:
  CliFunctorMap functor_map_;

  bool bind_get_token(string& str, string& token);
};

static inline unsigned int
masklen2mask(int masklen)
{
  return htonl(~((1 << (32 - masklen)) - 1));
}

#endif /* _CLI_UTILS_HPP_ */
