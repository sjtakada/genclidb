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

#include <iostream>
#include "project.hpp"

// Primitive string utility functions.

size_t
white_space_length(string& str)
{
  const char *p = str.c_str();

  while (*p != '\0')
    {
      if (!isspace(*p))
        break;
      p++;
    }

  return (size_t)(p - str.c_str());
}

size_t
command_length(string& str)
{
  const char *p = str.c_str();

  while (*p != '\0')
    {
      if (isspace(*p))
        break;
      p++;
    }

  return (size_t)(p - str.c_str());
}

bool
is_white_space_only(string& str)
{
  if (str.size() == 0)
    return true;

  return white_space_length(str) == str.size();
}

bool
trim_spaces_at_head(string& str)
{
  size_t len = white_space_length(str);

  if (len)
    {
      str = str.substr(len, string::npos);
      return true;
    }

  return false;
}

