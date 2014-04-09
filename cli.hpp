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

#include "cli_readline.hpp"
#include "cli_tree.hpp"

class Cli
{
public:
  static Cli *instance();
  static CliReadline& readline() { return Cli::instance()->rl_; }
  CliTree *current_mode() { return tree_["OSPF-NODE"]; }

  void init();
  void loop();
  int json_read(char *filename, Json::Value& root);

  void cli_read(char *filename);
  void cli_mode_read(char *filename);
  void cli_mode_traverse(Json::Value& current, CliTree *parent);
  //
  //  void readline_split(char *line, vector<string *>& tokens);
  //  int match_token(string *input, CliNode *node, CliNodeVector& matched);
  //  int readline_parse(CliNode *node, CliNodeVector& matched);

  //  int readline_describe();
  //  char **readline_completion(const char *text, int start, int end);
  //  char *readline_completion_matches(const char *text, int state);

private:
  // For singleton instance.
  Cli() { }
  Cli(Cli const&) { }

  // Singleton instance.
  static Cli *instance_;

  // Readline parser.
  CliReadline rl_;

  // CLI mode to tree map.
  typedef map<string, CliTree *> ModeTreeMap;
  ModeTreeMap tree_;

  // Member functions.
  //  void readline_init();
  //  char *readline_gets();
  //  void readline_execute() { };
  //  char *prompt();

  void cli_parse_command_all(Json::Value& tokens, Json::Value& commands);
  void cli_parse_command(Json::Value& tokens, Json::Value& command);
};

#endif /* _CLI_HPP_ */
