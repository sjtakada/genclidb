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

class CliNode;
typedef vector<CliNode *> CliNodeVector;

// Base virtual class for CLI Node.
class CliNode
{
public:
  CliNode() { }
  CliNode(int type, string& id, string& def_token, string& help)
    : type_(type), id_(id), def_token_(def_token), help_(help)
  { }
  ~CliNode() { }

  virtual const char *cli_token() { return cli_token_.c_str(); }
  virtual bool cli_match(string *input) { return true; }
  string& help() { return help_; }

  void sort_recursive();

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

  // Command
  bool cmd_;
};

// Keyword.
class CliNodeKeyword: public CliNode
{
public:
  CliNodeKeyword(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { cli_token_ = def_token; }

  bool cli_match(string *input)
  {
    return !input->compare(0, input->size(), cli_token(), input->size());
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

  bool cli_match(string *input);

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

  const char *cli_token() { return "A.B.C.D/M"; }

  bool cli_match(string *input);
};

// IPv4 Address.
class CliNodeIPv4Address: public CliNode
{
public:
  CliNodeIPv4Address(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { }

  const char *cli_token() { return "A.B.C.D"; }

  bool cli_match(string *input);
};

// IPv6 Prefix.
class CliNodeIPv6Prefix: public CliNode
{
public:
  CliNodeIPv6Prefix(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { }

  const char *cli_token() { return "X:X::X:X/M"; }
};

// IPv6 Address.
class CliNodeIPv6Address: public CliNode
{
public:
  CliNodeIPv6Address(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { }

  const char *cli_token() { return "X:X::X:X"; }
};

// Word.
class CliNodeWord: public CliNode
{
public:
  CliNodeWord(int type, string& id, string& def_token, string& help)
    : CliNode(type, id, def_token, help)
  { }

  const char *cli_token() { return "WORD"; }
};

// Per mode CLI Tree.
class CliTree
{
public:
  CliTree(const char *mode, CliTree *parent)
    : mode_(mode), parent_(parent)
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

  void build_command(Json::Value& defun, Json::Value& tokens);

  friend class CliReadline;

private:
  static const boost::regex re_keyword;
  static const boost::regex re_white_space;
  static const boost::regex re_vertical_bar;
  static const boost::regex re_left_paren;
  static const boost::regex re_right_paren;
  static const boost::regex re_left_bracket;
  static const boost::regex re_right_bracket;
  static const boost::regex re_left_brace;
  static const boost::regex re_right_brace;
  static const boost::regex re_ipv4_prefix;
  static const boost::regex re_ipv4_address;
  static const boost::regex re_ipv6_prefix;
  static const boost::regex re_ipv6_address;
  static const boost::regex re_range;
  static const boost::regex re_word;
  static const boost::regex re_array;

  // Mode name.
  string mode_;

  // Parent CLI tree.
  CliTree *parent_;

  // Member functions.
  int get_token(string& str, string& token);
  int build_recursive(CliNodeVector& curr, CliNodeVector& head,
                      CliNodeVector& tail, string& str, Json::Value& tokens);
  void vector_add_node_each(CliNodeVector& curr, CliNode *node);
  CliNode *new_node_by_type(int type, Json::Value& tokens, string& def_token);
  CliNode *find_next_by_def_token(CliNodeVector& v, string& token);
};

#endif /* _CLI_NODE_HPP_ */
