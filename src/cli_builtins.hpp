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
#ifndef _CLI_BUILTINS_HPP_
#define _CLI_BUILTINS_HPP_

#include <map>
#include <vector>

#include "project.hpp"

//void cli_builtins_init(Cli *cli);

class CliBuiltIn;

typedef int (*cli_builtin_func) (Cli *, StringVector& vec);
typedef map<string, cli_builtin_func> CliBuiltInFuncMap;

class CliBuiltIn
{
public:
  CliBuiltIn() { }
  void init(Cli *cli);
  int call(string name, StringVector& vec);

private:
  Cli *cli_;

  CliBuiltInFuncMap func_;

  static int debug_cli(Cli *cli, StringVector& vec);
  static int no_debug_cli(Cli *cli, StringVector& vec);
  static int exit(Cli *cli, StringVector& vec);
  static int show_result(Cli *cli, StringVector& vec);
  static int write_result(Cli *cli, StringVector& vec);
};

#endif /* _CLI_BUILTINS_HPP_ */
