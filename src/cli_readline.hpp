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
#ifndef _CLI_READLINE_HPP_
#define _CLI_READLINE_HPP_

#include <vector>
#include "cli_tree.hpp"
#include "cli_utils.hpp"

class Cli;

typedef pair<CliNode *, MatchState> CliNodeMatchStatePair;
typedef vector<CliNodeMatchStatePair> CliNodeMatchStateVector;

typedef pair<CliNode *, string *> CliNodeTokenPair;
typedef vector<CliNodeTokenPair> CliNodeTokenVector;

enum ExecResult {
  exec_complete = 0,
  exec_incomplete,
  exec_ambiguous,
  exec_unrecognized,
};

class CliReadline
{
public:
  CliReadline()
    : buf_(NULL), matched_strvec_(NULL), matched_index_(0) { }
  ~CliReadline() { }

  // Public member functions, Cli will call.
  void init(Cli *cli);
  char *gets();
  bool execute();
  int describe();
  char **completion(const char *text, int start, int end);
  char *completion_matches(const char *text, int state);

private:
  static const boost::regex re_white_space;
  static const boost::regex re_white_space_only;
  static const boost::regex re_command_string;

  // Parent CLI object.
  Cli *cli_;

  // Cli Utils.
  CliUtils utils_;

  // Readline buffer.
  char *buf_;
  
  // Completion matched string vector.
  char **matched_strvec_;
  int matched_index_;

  // Private member functions.
  bool get_token(string&str, string& token);
  bool skip_spaces(string& str);
  void fill_matched_vec(CliNode *node,
                        CliNodeMatchStateVector& matched_vec);
  void filter_matched(CliNodeMatchStateVector& matched_vec, MatchFlag limit);
  size_t match_token(string& input, CliNode *node,
                     CliNodeMatchStateVector& matched_vec);
  bool parse(string& line, CliNode *curr,
             CliNodeMatchStateVector& matched_vec);
  enum ExecResult parse_execute(string& line, CliNode *curr,
                                 CliNodeTokenVector& node_token_vec);
  void describe_line(CliNode *node, size_t max_len_token);
  void handle_actions(CliNodeTokenVector& node_token_vec);
};

#endif /* _CLI_READLINE_HPP_ */
