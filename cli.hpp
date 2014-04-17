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
#ifndef _CLI_HPP_
#define _CLI_HPP_

using namespace std;

#include <sys/ioctl.h>
#include <stdio.h>

#include "cli_readline.hpp"
#include "cli_tree.hpp"

class Cli
{
public:
  static Cli *instance();
  static CliReadline& readline() { return Cli::instance()->rl_; }
  CliTree *current_tree() { return tree_[mode_]; }

  void terminal_init();
  void init();
  void loop();
  int json_read(char *filename, Json::Value& root);

  void cli_read(char *filename);
  void mode_read(char *filename);
  void mode_traverse(Json::Value& current, CliTree *parent);

  // Terminal width, height.
  struct winsize ws_;

private:
  // For singleton instance.
  Cli() { }
  Cli(Cli const&) { }

  // Singleton instance.
  static Cli *instance_;

  // Current mode string.
  string mode_;

  // Readline parser.
  CliReadline rl_;

  // CLI mode to tree map.
  typedef map<string, CliTree *> ModeTreeMap;
  ModeTreeMap tree_;

  // Member functions.
  void signal_init();
  void parse_defun_all(Json::Value& tokens, Json::Value& commands);
  void parse_defun(Json::Value& tokens, Json::Value& command);
};

#endif /* _CLI_HPP_ */
