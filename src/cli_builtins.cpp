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
cli_builtins_debug_cli_set(Cli *cli, StringVector& vec)
{
  cli->set_debug(true);

  return 1;
}

int
cli_builtins_debug_cli_unset(Cli *cli, StringVector& vec)
{
  cli->set_debug(false);

  return 1;
}

int
cli_exit(Cli *cli, StringVector& vec)
{
  cli->exit();

  return 1;
}

void
cli_builtins_init(Cli *cli)
{
  cli->built_in_["debug-cli-set"] = cli_builtins_debug_cli_set;
  cli->built_in_["debug-cli-unset"] = cli_builtins_debug_cli_unset;
  cli->built_in_["cli-exit"] = cli_exit;
}

void
cli_mode_set(Cli *cli, string name)
{
  cli->mode_set(name);
}

void
cli_mode_up(Cli *cli, unsigned int up)
{
  cli->mode_up(up);
}
