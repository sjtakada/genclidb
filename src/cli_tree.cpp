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
#include <algorithm>

#include "boost/regex.hpp"
#include "json/json.h"

#include "project.hpp"
#include "cli_tree.hpp"

// Regexp to parse CLI JSON defun.
const boost::regex CliTree::re_keyword("^([a-z0-9\\*][a-z_A-Z0-9\\-]*)");
const boost::regex CliTree::re_white_space("^( +)");
const boost::regex CliTree::re_vertical_bar("^(\\|)");
const boost::regex CliTree::re_left_paren("^(\\()");
const boost::regex CliTree::re_right_paren("^(\\))");
const boost::regex CliTree::re_left_brace("^(\\{)");
const boost::regex CliTree::re_right_brace("^(\\})");
const boost::regex CliTree::re_left_bracket("^(\\[)");
const boost::regex CliTree::re_right_bracket("^(\\])");
const boost::regex CliTree::re_ipv4_prefix("^(IPV4-PREFIX:[0-9\\.]+)");
const boost::regex CliTree::re_ipv4_address("^(IPV4-ADDR:[0-9\\.]+)");
const boost::regex CliTree::re_ipv6_prefix("(^IPV6-PREFIX:[0-9\\.]+)");
const boost::regex CliTree::re_ipv6_address("^(IPV6-ADDR:[0-9\\.]+)");
const boost::regex CliTree::re_range("^(RANGE:[0-9\\.]+)");
const boost::regex CliTree::re_metric_offset("^(METRIC-OFFSET:[0-9\\.]+)");
const boost::regex CliTree::re_community_new("^(COMMUNITY:[0-9\\.]+)");
const boost::regex CliTree::re_word("^(WORD:[0-9\\.]+)");
const boost::regex CliTree::re_time("^(TIME:[0-9\\.]+)");
const boost::regex CliTree::re_month("^(MONTH:[0-9\\.]+)");
const boost::regex CliTree::re_array("^(ARRAY:[0-9\\.]+)");

int
CliTree::get_token(string& str, string& token)
{
  boost::smatch m;
  int type = CliTree::undef;

  // First skip whitespace.
  if (boost::regex_search(str, m, re_white_space))
    str = m.suffix();

  if (boost::regex_search(str, m, re_vertical_bar))
    type = CliTree::vertical_bar;
  else if (boost::regex_search(str, m, re_left_paren))
    type = CliTree::left_paren;
  else if (boost::regex_search(str, m, re_right_paren))
    type = CliTree::right_paren;
  else if (boost::regex_search(str, m, re_left_brace))
    type = CliTree::left_brace;
  else if (boost::regex_search(str, m, re_right_brace))
    type = CliTree::right_brace;
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
  else if (boost::regex_search(str, m, re_metric_offset))
    type = CliTree::word; // TODO
  else if (boost::regex_search(str, m, re_community_new))
    type = CliTree::word; // TODO
  else if (boost::regex_search(str, m, re_word))
    type = CliTree::word;
  else if (boost::regex_search(str, m, re_time))
    type = CliTree::word; // TODO
  else if (boost::regex_search(str, m, re_month))
    type = CliTree::word; // TODO
  else if (boost::regex_search(str, m, re_array))
    type = CliTree::array;
  else if (boost::regex_search(str, m, re_keyword))
    type = CliTree::keyword;
  else
    {
      cout << "no match" << endl;
      assert(0);
    }

  token = m[0];
  str = m.suffix();

  return type;
}

CliNode *
CliTree::find_next_by_def_token(CliNodeVector& v, string& token)
{
  CliNode *node;

  for (CliNodeVector::iterator it = v.begin(); it != v.end(); ++it)
    {
      node = (*it);
      for (CliNodeVector::iterator is = node->next_.begin();
         is != node->next_.end(); ++is)
        if ((*is)->def_token_ == token)
          return (*is);
    }

  return NULL;
}

CliNode *
CliTree::new_node_by_type(int type, Json::Value& tokens, string& def_token)
{
  string id = tokens[def_token]["id"].asString();
  string help = tokens[def_token]["help"].asString();
  CliNode *node;

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
    default:
      cout << "Unknown type" << endl;
      assert(0);
    }

  return node;
}

// Link given node to each next node in current vector.
void
CliTree::vector_add_node_each(CliNodeVector& curr, CliNode *node)
{
  for (CliNodeVector::iterator it = curr.begin(); it != curr.end(); ++it)
    (*it)->next_.push_back(node);
}

int
CliTree::build_recursive(CliNodeVector& curr,
                         CliNodeVector& head,
                         CliNodeVector& tail,
                         string& str, Json::Value& tokens)
{
  CliNode *next;
  string def_token;
  int type;
  bool is_head = true;

  while (str.begin() != str.end())
    {
      type = get_token(str, def_token);
      switch (type)
        {
        case CliTree::left_paren:
        case CliTree::left_bracket:
        case CliTree::left_brace:
          {
            CliNodeVector hv, tv;

            do
              {
                CliNodeVector cv(curr);
                type = build_recursive(cv, hv, tv, str, tokens);
              }
            while (type == CliTree::vertical_bar);

            if (type == CliTree::right_brace)
              {
                for (CliNodeVector::iterator it = hv.begin();
                     it != hv.end(); ++it)
                  vector_add_node_each(tv, *it);
              }

            curr = tv;
          }
          break;
        case CliTree::vertical_bar:
        case CliTree::right_paren:
        case CliTree::right_bracket:
        case CliTree::right_brace:
          copy(curr.begin(), curr.end(), back_inserter(tail));
          return type;

        default:
          next = find_next_by_def_token(curr, def_token);
          if (next == NULL)
            {
              next = new_node_by_type(type, tokens, def_token);
              vector_add_node_each(curr, next);
            }

          // Moving forward.
          curr.clear();
          curr.push_back(next);

          if (is_head)
            {
              head.push_back(next);
              is_head = false;
            }
          break;
        }
    }

  // TODO
  for (CliNodeVector::iterator it = curr.begin(); it != curr.end(); ++it)
    (*it)->cmd_ = true;

  return CliTree::undef;
}

void
CliTree::build_command(Json::Value& defun, Json::Value& tokens)
{
  string str(defun.asString());
  CliNodeVector cv, hv, tv;
  cv.push_back(top_);

  build_recursive(cv, hv, tv, str, tokens);
}


bool
CliNodeCriterion(CliNode *n, CliNode *m)
{
  return strcmp(n->cli_token(), m->cli_token()) < 0;
}

void
CliNode::sort_recursive()
{
  sort(next_.begin(), next_.end(), CliNodeCriterion);
  sorted_ = true;

  for (CliNodeVector::iterator it = next_.begin(); it != next_.end(); ++it)
    if (!(*it)->sorted_)
      (*it)->sort_recursive();
}

MatchState
CliNodeRange::cli_match(string& input)
{
  u_int64_t val;
  char *endptr = NULL;

  val = strtoull(input.c_str(), &endptr, 10);
  if (*endptr != '\0' || val < min_ || val > max_)
    return match_none;

  return match_full;
}

MatchState
CliNodeIPv4Prefix::cli_match(string& input)
{
  size_t s = 0;
  size_t pos;
  size_t length = input.size();

  for (int i = 0; i < 4; i++)
    {
      u_int16_t val;
      char *endptr = NULL;

      pos = input.find('.', s);
      if (pos == string::npos)
        pos = length;

      string sub = input.substr(s, pos);
      val = strtoul(sub.c_str(), &endptr, 10);
      if (*endptr != '\0' || val > 255)
        return match_none;

      if (pos == length)
        break;
    }

  return match_full;
}

MatchState
CliNodeIPv4Address::cli_match(string& input)
{
  const char *str = input.c_str();
  const char *p = str;

  for (int i = 0; i < 4; i++)
    {
      u_int16_t val = 0;

      // Not a number.
      if (*p < '0' || *p > '9')
        return match_none;

      val = *p - '0';
      p++;

      while (*p != '\0')
        {
          if (*p == '.')
            {
              if (i == 3)
                return match_none;

              p++;
              break;
            }

          // Not a number.
          if (*p < '0' || *p > '9')
            return match_none;

          val = val * 10 + (*p - '0');
          if (val > 255)
            return match_none;

          p++;
        }

      if (*p == '\0')
        {
          if (i == 3)
            return match_full;
          else
            return match_incomplete;
        }
    }

  return match_none;
}

