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

#include "project.hpp"
#include "cli.hpp"
#include "cli_builtins.hpp"

int
CliBuiltIn::debug_cli(Cli *cli, StringVector& vec)
{
  cli->set_debug(true);

  return 1;
}

int
CliBuiltIn::no_debug_cli(Cli *cli, StringVector& vec)
{
  cli->set_debug(false);

  return 1;
}

int
CliBuiltIn::exit(Cli *cli, StringVector& vec)
{
  cli->exit();

  return 1;
}

int
CliBuiltIn::show_result(Cli *cli, StringVector& vec)
{
  cout << cli->result().str() << endl;

  return 1;
}

int
CliBuiltIn::write_result(Cli *cli, StringVector& vec)
{
  ofstream file;

  file.open("error.txt");
  file << cli->result().str() << endl;
  file.close();

  return 1;
}


void
CliBuiltIn::init(Cli *cli)
{
  cli_ = cli;

  func_["debug-cli"] = debug_cli;
  func_["no-debug-cli"] = no_debug_cli;
  func_["exit"] = exit;
  func_["show-result"] = show_result;
  func_["write-result"] = write_result;
}

int
CliBuiltIn::call(string name, StringVector& vec)
{
  CliBuiltInFuncMap::iterator it = func_.find(name);
  if (it != func_.end())
    return it->second(cli_, vec);

  return 0;
}
