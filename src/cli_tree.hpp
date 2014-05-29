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
#ifndef _CLI_NODE_HPP_
#define _CLI_NODE_HPP_

#include "project.hpp"
#include "json/json.h"
#include "boost/regex.hpp"

class CliAction;
class CliNode;
typedef vector<CliNode *> CliNodeVector;
typedef pair<string, string> CondBindPair;
typedef vector<CondBindPair> CondBindPairVector;

// Simply whether match or not.
enum MatchResult {
  match_failure = 0,
  match_success = 1,
};

// Flag if matched.
enum MatchFlag {
  match_full = 0,	// Fully matched.
  match_partial = 1,	// Partially matched, still valid.
  match_incomplete = 2,	// String incomplete, not valid for execution.
  match_none = 3,
};

typedef pair<enum MatchResult, enum MatchFlag> MatchState;
typedef vector<CliAction *> CliActionVector;

// Base virtual class for CLI Node.
class CliNode
{
public:
  CliNode() { }
  CliNode(int type, string& id, string& def_token, string& help)
    : type_(type), id_(id), def_token_(def_token), help_(help),
      cli_token_(""), completable_(false), cmd_(false)
  { }
  ~CliNode() { }

  virtual const string& cli_token() { return cli_token_; }
  virtual MatchState cli_match(string& input)
  { return make_pair(match_failure, match_none); }
  string& help() { return help_; }
  virtual string& format_param(string& input) { return input; }

  void sort_recursive();
  string& def_token() { return def_token_; }

  friend class CliTree;
  friend class CliReadline;

private:
  // Node type.
  int type_;

  // Node vector is sorted.
  bool sorted_;

protected:
  // Node ID.
  string id_;

  // Defun token.
  string def_token_;

  // Help string.
  string help_;

  // CLI token.
  string cli_token_;

  // Next candidates.
  CliNodeVector next_;

  // It can complete.
  bool completable_;

  // Bind-if preprocess statement.
  CondBindPairVector bind_if_;

  // Command.
  bool cmd_;

  // Action.
  CliActionVector actions_;
};

// Keyword.
class CliNodeKeyword: public CliNode
{
public:
  CliNodeKeyword(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { cli_token_ = def_token;
    completable_ = true; }

  MatchState cli_match(string& input)
  {
    if (input == cli_token())
      return make_pair(match_success, match_full);

    if (!input.compare(0, input.size(), cli_token(), 0, input.size()))
      return make_pair(match_success, match_partial);

    return make_pair(match_failure, match_none);
  }
};

// Integer Range.
class CliNodeRange: public CliNode
{
public:
  CliNodeRange(int type, string& id, string& def_token, string& help,
               u_int64_t min, u_int64_t max)
    : CliNode(type, id, def_token, help), min_(min), max_(max)
  {
    ostringstream ss;

    ss << "<" << min_ << "-" << max_ << ">";
    cli_token_ = ss.str();
  }

  MatchState cli_match(string& input);

private:
  u_int64_t min_;
  u_int64_t max_;
};

// IPv4 Prefix.
class CliNodeIPv4Prefix: public CliNode
{
public:
  CliNodeIPv4Prefix(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { }

  const string& cli_token() { return CliNodeIPv4Prefix::cli_token_default_; } 
  MatchState cli_match(string& input);

private:
  const static string cli_token_default_;
};

// IPv4 Address.
class CliNodeIPv4Address: public CliNode
{
public:
  CliNodeIPv4Address(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { }

  const string& cli_token() { return CliNodeIPv4Address::cli_token_default_; } 
  MatchState cli_match(string& input);

private:
  const static string cli_token_default_;
};

// IPv6 Prefix.
class CliNodeIPv6Prefix: public CliNode
{
public:
  CliNodeIPv6Prefix(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { }

  const string& cli_token() { return CliNodeIPv6Prefix::cli_token_default_; } 
  MatchState cli_match(string& input);

private:
  const static string cli_token_default_;
};

// IPv6 Address.
class CliNodeIPv6Address: public CliNode
{
public:
  CliNodeIPv6Address(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { }

  const string& cli_token() { return CliNodeIPv6Address::cli_token_default_; } 
  MatchState cli_match(string& input);

private:
  const static string cli_token_default_;
};

// Word.
class CliNodeWord: public CliNode
{
public:
  CliNodeWord(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { cli_token_ = "WORD"; }

  const string& cli_token() { return CliNodeWord::cli_token_default_; }
  MatchState cli_match(string& input)
  { return make_pair(match_success, match_partial); }

private:
  const static string cli_token_default_;
};

// Per mode CLI Tree.
class CliTree
{
public:
  CliTree(const char *mode, string& prompt, CliTree *parent)
    : mode_(mode), prompt_(prompt), parent_(parent)
  { top_ = new CliNode; }
  ~CliTree() { }

  // CLI top node.
  CliNode *top_;

  enum {
    undef,
    white_space,
    vertical_bar,
    left_paren,
    right_paren,
    left_bracket,
    right_bracket,
    left_brace,
    right_brace,
    ipv4_prefix,
    ipv4_address,
    ipv6_prefix,
    ipv6_address,
    range,
    word,
    array,
    keyword,
  } token;

  void build_command(Json::Value& tokens, Json::Value& command);

  string& prompt() { return prompt_; }

  friend class CliReadline;

private:
  static const boost::regex re_white_space;

  // Mode name.
  string mode_;

  // Prompt.
  string prompt_;

  // Parent CLI tree.
  CliTree *parent_;

  // Member functions.
  int get_token(string& str, string& token);
  int build_recursive(CliNodeVector& curr, CliNodeVector& head,
                      CliNodeVector& tail, string& str, Json::Value& tokens,
                      Json::Value& command);
  void vector_add_node_each(CliNodeVector& curr, CliNode *node);
  CliNode *new_node_by_type(int type, Json::Value& tokens, string& def_token);
  CliNode *find_next_by_node(CliNodeVector& v, CliNode *node);
};

#endif /* _CLI_NODE_HPP_ */
