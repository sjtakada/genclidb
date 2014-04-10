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
#include "boost/regex.hpp"
#include "json/json.h"

#include "project.hpp"
#include "cli_tree.hpp"

// Regexp to parse CLI JSON defun.
const boost::regex CliTree::re_keyword("^([a-z0-9][a-z_A-Z0-9\\-]*)");
const boost::regex CliTree::re_whitespace("^( +)");
const boost::regex CliTree::re_pipe("^(\\|)");
const boost::regex CliTree::re_leftparen("^(\\()");
const boost::regex CliTree::re_rightparen("^(\\))");
const boost::regex CliTree::re_left_bracket("^(\\[)");
const boost::regex CliTree::re_right_bracket("^(\\])");
const boost::regex CliTree::re_ipv4_prefix("^(IPV4-PREFIX:[0-9\\.]+)");
const boost::regex CliTree::re_ipv4_address("(^IPV4-ADDR:[0-9\\.]+)");
const boost::regex CliTree::re_ipv6_prefix("(^IPV6-PREFIX:[0-9\\.]+)");
const boost::regex CliTree::re_ipv6_address("(^IPV6-ADDR:[0-9\\.]+)");
const boost::regex CliTree::re_range("(^RANGE:[0-9\\.]+)");
const boost::regex CliTree::re_word("(^WORD:[0-9\\.]+)");
const boost::regex CliTree::re_array("(^ARRAY:[0-9\\.]+)");

int
CliTree::get_token(string& str, string& token)
{
  boost::smatch m;
  int type = 0;

  if (boost::regex_search(str, m, re_whitespace))
    type = CliTree::white_space;
  else if (boost::regex_search(str, m, re_pipe))
    type = CliTree::pipe;
  else if (boost::regex_search(str, m, re_leftparen))
    type = CliTree::left_paren;
  else if (boost::regex_search(str, m, re_rightparen))
    type = CliTree::right_paren;
  else if (boost::regex_search(str, m, re_left_bracket))
    type = CliTree::left_bracket;
  else if (boost::regex_search(str, m, re_right_bracket))
    type = CliTree::right_bracket;
  else if (boost::regex_search(str, m, re_ipv4_prefix))
    type = CliTree::ipv4_prefix;
  else if (boost::regex_search(str, m, re_ipv4_address))
    type = CliTree::ipv4_address;
  else if (boost::regex_search(str, m, re_ipv6_prefix))
    type = CliTree::ipv6_prefix;
  else if (boost::regex_search(str, m, re_ipv6_address))
    type = CliTree::ipv6_address;
  else if (boost::regex_search(str, m, re_range))
    type = CliTree::range;
  else if (boost::regex_search(str, m, re_word))
    type = CliTree::word;
  else if (boost::regex_search(str, m, re_array))
    type = CliTree::array;
  else if (boost::regex_search(str, m, re_keyword))
    type = CliTree::keyword;
  else
    {
      cout << "no match";
      abort();
    }

  token = m[0];
  str = m.suffix();

  return type;
}

CliNode *
CliTree::find_by_def_token(CliNode *current, string& token)
{
  CliNode *node;

  for (CliNodeVector::iterator it = current->next_.begin();
       it != current->next_.end(); ++it)
    {
      node = (*it);

      if (node->def_token_ == token)
        return node;
    }

  return NULL;
}

void
CliTree::build(CliNode *current, string& str, Json::Value& tokens)
{
  string def_token;
  int type;

  while (str.begin() != str.end())
    {
      type = get_token(str, def_token);
      switch (type)
        {
        case CliTree::white_space:
          continue;
        case CliTree::pipe:
          continue;
        case CliTree::left_paren:
        case CliTree::left_bracket:
        case CliTree::left_brace:


          continue;
        case CliTree::right_paren:
        case CliTree::right_bracket:
        case CliTree::right_brace:
          continue;
        case CliTree::ipv4_prefix:
        case CliTree::ipv4_address:
        case CliTree::ipv6_prefix:
        case CliTree::ipv6_address:
        case CliTree::range:
        case CliTree::word:
        case CliTree::array:
        case CliTree::keyword:
          break;
        }

      CliNode *node = find_by_def_token(current, def_token);
      if (node == NULL)
        {
          string id = tokens[def_token]["id"].asString();
          string help = tokens[def_token]["help"].asString();

          switch (type)
            {
            case CliTree::ipv4_prefix:
              node = new CliNodeIPv4Prefix(type, id, def_token, help);
              break;
            case CliTree::ipv4_address:
              node = new CliNodeIPv4Address(type, id, def_token, help);
              break;
            case CliTree::ipv6_prefix:
              node = new CliNodeIPv6Prefix(type, id, def_token, help);
              break;
            case CliTree::ipv6_address:
              node = new CliNodeIPv6Address(type, id, def_token, help);
              break;
            case CliTree::range:
              {
                Json::Value range = tokens[def_token]["range"];
                u_int64_t min = range[(Json::Value::ArrayIndex)0].asUInt();
                u_int64_t max = range[(Json::Value::ArrayIndex)1].asUInt();
                node = new CliNodeRange(type, id, def_token, help,
                                        min, max);
                break;
              }
            case CliTree::word:
              node = new CliNodeWord(type, id, def_token, help);
              break;
            case CliTree::array:
              node = new CliNodeWord(type, id, def_token, help);
              break;
            case CliTree::keyword:
              node = new CliNodeKeyword(type, id, def_token, help);
              break;
            }

          current->next_.push_back(node);
        }

      current = node;
    }
}

void
CliTree::parse_defun(Json::Value& defun, Json::Value& tokens)
{
  string str(defun.asString());

  build(node_, str, tokens);
}

bool
CliNodeRange::cli_match(string *input)
{
  u_int64_t val;
  char *endptr = NULL;

  val = strtoull(input->c_str(), &endptr, 10);
  if (*endptr != '\0' || val < min_ || val > max_)
    return false;

  return true;
}

bool
CliNodeIPv4Prefix::cli_match(string *input)
{
  size_t s = 0;
  size_t pos;
  size_t length = input->size();

  for (int i = 0; i < 4; i++)
    {
      u_int16_t val;
      char *endptr = NULL;

      pos = input->find('.', s);
      if (pos == string::npos)
        pos = length;

      string sub = input->substr(s, pos);
      val = strtoul(sub.c_str(), &endptr, 10);
      if (*endptr != '\0' || val > 255)
        return false;

      if (pos == length)
        break;
    }

  return true;
}

bool
CliNodeIPv4Address::cli_match(string *input)
{
  const char *str = input->c_str();
  const char *p = str;

  for (int i = 0; i < 4; i++)
    {
      u_int16_t val = 0;

      // Not a number.
      if (*p < '0' || *p > '9')
        return false;

      val = *p - '0';
      p++;

      while (*p != '\0')
        {
          if (*p == '.')
            {
              if (i == 3)
                return false;

              p++;
              break;
            }

          // Not a number.
          if (*p < '0' || *p > '9')
            return false;

          val = val * 10 + (*p - '0');
          if (val > 255)
            return false;

          p++;
        }

      if (*p == '\0')
        return true;
    }

  return false;
}
