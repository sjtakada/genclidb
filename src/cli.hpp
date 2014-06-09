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
#include "cli_builtins.hpp"



class Cli
{
public:
  static Cli *instance();
  static CliReadline& readline() { return Cli::instance()->rl_; }
  CliTree *current_mode() { return mode_; }
  void set_debug(bool debug) { debug_ = debug; }
  bool is_debug() { return debug_; }

  void terminal_init();
  void init();
  void loop();
  int json_read(char *filename, Json::Value& root);

  void load_cli_json(char *filename);
  bool load_cli_json_all(char *dirname);
  void mode_read(char *filename);
  CliTree *mode_traverse(Json::Value& current, CliTree *parent);
  bool mode_set(string& mode_str);
  bool mode_up(unsigned int up);
  void exit();
  void end();
  void start_over();
  const char *prompt();

  // Terminal width, height.
  struct winsize ws_;

  map<string, cli_builtin_func> built_in_;

  // Mode parameters.
  ParamsMap params_;

private:
  // For singleton instance.
  Cli() : debug_(true), exit_(false), mode_(NULL), hostname_("Router") { }
  Cli(Cli const&) { }

  // Singleton instance.
  static Cli *instance_;

  // Debug mode.
  bool debug_;
  bool exit_;

  // Current mode.
  CliTree *mode_;

  // Current path.
  //  StringVector paths_;

  // Readline parser.
  CliReadline rl_;

  // CLI mode to tree map.
  typedef map<string, CliTree *> ModeTreeMap;
  ModeTreeMap tree_;

  // Hostname.
  string hostname_;

  // Privileged mode name.
  string privileged_mode_;

  // Member functions.
  void signal_init();
  void parse_defun_all(Json::Value& tokens, Json::Value& commands);
  void parse_defun(Json::Value& tokens, Json::Value& command);
};

#endif /* _CLI_HPP_ */
