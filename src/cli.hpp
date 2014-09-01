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

  void set_cli_mode_file(char *file) { cli_mode_file_ = file; }
  string& cli_mode_file() { return cli_mode_file_; }

  void set_cli_json_dir(char *dir) { cli_json_dir_ = dir; }
  string& cli_json_dir() { return cli_json_dir_; }

  void set_api_server(char *api_server) { api_server_ = api_server; }
  string& api_server() { return api_server_; }

  void set_api_prefix(char *api_prefix) { api_prefix_ = api_prefix; }
  string& api_prefix() { return api_prefix_; }

  void set_null_key(char *null_key) { null_key_ = null_key; }
  string& null_key() { return null_key_; }

  void terminal_init();
  bool init();
  void loop();
  int json_read(char *filename, Json::Value& root);

  void load_cli_json(char *filename);
  bool load_cli_json_all(char *dirname);
  int mode_read(char *filename);
  CliTree *mode_traverse(Json::Value& current, CliTree *parent);
  bool mode_set(string& mode_str);
  bool mode_set(CliTree *mode);
  bool mode_up(unsigned int up);
  void exit();
  void end();
  void start_over();
  const char *prompt();
  stringstream& result() { return result_; }
  void set_result(stringstream& result) { result_.str(result.str()); }

  void params_populate(ParamsMap& input)
  {
    for (ParamsMap::iterator it = params_.begin(); it != params_.end(); ++it)
      input[it->first] = it->second;
  }

  // Terminal width, height.
  struct winsize ws_;

  // Built-in function map.
  CliBuiltIn built_in_;

  // Mode parameters.
  ParamsMap params_;

private:
  // For singleton instance.
  Cli() : debug_(true), exit_(false), cli_mode_file_("cli_mode.json"),
          cli_json_dir_("."), api_server_("localhost"), api_prefix_("api"),
          mode_(NULL), hostname_("Router") { }
  Cli(Cli const&) { }

  // Singleton instance.
  static Cli *instance_;

  // Debug mode.
  bool debug_;

  // Exit flag.
  bool exit_;

  // CLI mode file.
  string cli_mode_file_;

  // CLI JSON directory.
  string cli_json_dir_;

  // API server host.
  string api_server_;

  // API prefix.
  string api_prefix_;

  // Null Key string
  string null_key_;

  // Current mode.
  CliTree *mode_;

  // Readline parser.
  CliReadline rl_;

  // CLI mode to tree map.
  typedef map<string, CliTree *> ModeTreeMap;
  ModeTreeMap tree_;

  // Hostname.
  string hostname_;

  // Privileged mode name.
  string privileged_mode_;

  // Result Buffer.
  stringstream result_;

  // Member functions.
  void signal_init();
  void parse_defun_all(Json::Value& tokens, Json::Value& commands);
  void parse_defun(Json::Value& tokens, Json::Value& command);
};

#endif /* _CLI_HPP_ */
