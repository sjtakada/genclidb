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
#include <arpa/inet.h>
#include <algorithm>

#include "boost/regex.hpp"
#include "json/json.h"

#include "project.hpp"
#include "cli.hpp"
#include "cli_tree.hpp"
#include "cli_action.hpp"
#include "cli_string.hpp"

const string CliNodeIPv4Prefix::cli_token_default_("A.B.C.D/M");
const string CliNodeIPv4Address::cli_token_default_("A.B.C.D");
const string CliNodeIPv6Prefix::cli_token_default_("X:X::X:X/M");
const string CliNodeIPv6Address::cli_token_default_("X:X::X:X");
const string CliNodeWord::cli_token_default_("WORD");

int
CliTree::get_token(string& str, string& token)
{
  const char *reject = "()[]{}| ";
  boost::smatch m;
  int type = CliTree::undef;
  const char *p;
  size_t offset = 0;

  // First skip whitespace.
  trim_spaces_at_head(str);

  p = str.c_str();
  if (*p == '|')
    {
      offset = 1;
      type = CliTree::vertical_bar;
    }
  else if (*p == '(')
    {
      offset = 1;
      type = CliTree::left_paren;
    }
  else if (*p == ')')
    {
      offset = 1;
      type = CliTree::right_paren;
    }
  else if (*p == '{')
    {
      offset = 1;
      type = CliTree::left_brace;
    }
  else if (*p == '}')
    {
      offset = 1;
      type = CliTree::right_brace;
    }
  else if (*p == '[')
    {
      offset = 1;
      type = CliTree::left_bracket;
    }
  else if (*p == ']')
    {
      offset = 1;
      type = CliTree::right_bracket;
    }
  else
    {
      offset = strcspn(p, reject);

      if (*p < 'A' || *p > 'Z')
        type = CliTree::keyword;
      else if (strncmp(p, "IPV4-PREFIX", 11) == 0)
        type = CliTree::ipv4_prefix;
      else if (strncmp(p, "IPV4-ADDR", 9) == 0)
        type = CliTree::ipv4_address;
      else if (strncmp(p, "IPV6-PREFIX", 11) == 0)
        type = CliTree::ipv6_prefix;
      else if (strncmp(p, "IPV6-ADDR", 9) == 0)
        type = CliTree::ipv6_address;
      else if (strncmp(p, "RANGE", 5) == 0)
        type = CliTree::range;
      else if (strncmp(p, "METRIC-OFFSET", 13) == 0)
        type = CliTree::word;
      else if (strncmp(p, "COMMUNITY", 9) == 0)
        type = CliTree::word;
      else if (strncmp(p, "WORD", 4) == 0)
        type = CliTree::word;
      else if (strncmp(p, "TIME", 4) == 0)
        type = CliTree::word;
      else if (strncmp(p, "MONTH", 5) == 0)
        type = CliTree::word;
      else if (strncmp(p, "ARRAY", 5) == 0)
        type = CliTree::word;
      else
        {
          cout << "Error: unknown token " << p << endl;
          assert(0);
        }
    }  

  token = str.substr(0, offset);
  str = str.substr(offset, string::npos);

  return type;
}

CliNode *
CliTree::find_next_by_node(CliNodeVector& v, CliNode *new_node)
{
  CliNode *node;

  for (CliNodeVector::iterator it = v.begin(); it != v.end(); ++it)
    {
      node = (*it);
      for (CliNodeVector::iterator is = node->next_.begin();
         is != node->next_.end(); ++is)
        if ((*is)->cli_token() == new_node->cli_token())
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
      {
        string enum_key;
        if (!tokens[def_token]["enum"].isNull())
          enum_key = tokens[def_token]["enum"].asString();

        node = new CliNodeKeyword(type, id, def_token, help, enum_key);
        break;
      }
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
CliTree::build_recursive(CliNodeVector& curr, CliNodeVector& head,
                         CliNodeVector& tail, string& str,
                         Json::Value& tokens, Json::Value& command)
{
  CliNode *next, *node;
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
                type = build_recursive(cv, hv, tv, str, tokens, command);
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
          node = new_node_by_type(type, tokens, def_token);
          next = find_next_by_node(curr, node);
          if (next == NULL)
            {
              vector_add_node_each(curr, node);
              next = node;
            }
          else
            delete node;

          // Merge exposed count.
          if (!tokens[def_token]["hidden"].isNull())
            {
              if (!tokens[def_token]["hidden"].asBool())
                next->exposed_++;
            }
          else
            next->exposed_++;

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

  for (CliNodeVector::iterator it = curr.begin(); it != curr.end(); ++it)
    {
      node = *it;
      node->cmd_ = true;

      Json::Value bind = command["bind"];
      if (!bind.isNull())
        {
          for (Json::Value::iterator is = bind.begin(); is != bind.end(); ++is)
            {
              Json::Value obj = *is;
              string exp = obj.asString();
              size_t pos;
              string lvalue, rvalue;

              pos = exp.find(" do ");
              if (pos != string::npos)
                {
                  lvalue = exp.substr(0, pos);
                  rvalue = exp.substr(pos + 4);
                }
              else
                {
                  lvalue = "";
                  rvalue = exp;
                }

              //              cout << "lvalue: '" << lvalue << "'" << endl;
              //              cout << "rvalue: '" << rvalue << "'" << endl;

              if (lvalue != "")
                {
                  pos = lvalue.find("if ");
                  if (pos != string::npos)
                    lvalue = lvalue.substr(pos + 3);

                  pos = lvalue.find(" ");
                  if (pos != string::npos)
                    lvalue = lvalue.substr(0, pos);
                }
                  
              CondBindPair p = make_pair(lvalue, rvalue);
              node->bind_.push_back(p);
            }
        }

      Json::Value actions = command["action"];
      if (!actions.isNull())
        for (Json::Value::iterator is = actions.begin();
             is != actions.end(); ++is)
          {
            Json::Value action = *is;
            string cond("*");

            if (!action["cond"].isNull())
              cond = action["cond"].asString();

            if (!action.isNull())
              {
                Json::Value http = action["http"];
                Json::Value mode = action["mode"];
                Json::Value built_in = action["built-in"];

                if (!http.isNull())
                  {
                    CondCliActionPair p =
                      make_pair(cond, new CliActionHttp(http));
                    node->actions_.push_back(p);
                  }

                if (!mode.isNull())
                  {
                    CondCliActionPair p =
                      make_pair(cond, new CliActionMode(mode));
                    node->actions_.push_back(p);
                  }

                if (!built_in.isNull())
                  {
                    CondCliActionPair p =
                      make_pair(cond, new CliActionBuiltIn(built_in));
                    node->actions_.push_back(p);
                  }
              }
          }
    }

  return CliTree::undef;
}

void
CliTree::build_command(Json::Value& tokens, Json::Value& command)
{
  Json::Value defun = command["defun"];
  if (!defun.isNull())
    {
      string str(defun.asString());
      CliNodeVector cv, hv, tv;
      cv.push_back(top_);

      build_recursive(cv, hv, tv, str, tokens, command);
    }
}


bool
CliNodeCriterion(CliNode *n, CliNode *m)
{
  return n->cli_token() < m->cli_token();
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
    return make_pair(match_failure, match_none);

  return make_pair(match_success, match_full);
}

MatchState
CliNodeKeyword::cli_match(string& input)
{
  if (input == cli_token())
    return make_pair(match_success, match_full);

  if (!input.compare(0, input.size(), cli_token(), 0, input.size()))
    return make_pair(match_success, match_partial);

  return make_pair(match_failure, match_none);
}

MatchState
CliNodeIPv4Prefix::cli_match(string& input)
{
  const char *str = input.c_str();
  const char *p = str;
  int val;
  int dots = 0;
  int octets = 0;
  int plen;

  enum {
    state_init,
    state_digit,
    state_dot,
    state_slash,
    state_plen
  };

  int state = state_init;

  while (*p != '\0')
    {
      switch (state)
        {
        case state_init:
          if (!isdigit((int)*p))
            return make_pair(match_failure, match_none);;

          state = state_digit;
          octets++;
          val = (int)(*p - '0');
          break;
        case state_digit:
          if (!isdigit((int)*p) && *p != '.' && *p != '/')
            return make_pair(match_failure, match_none);

          if (*p == '.')
            {
              state = state_dot;
              dots++;
              if (dots > 3)
                return make_pair(match_failure, match_none);;
            }
          else if (*p == '/')
            state = state_slash;
          else
            {
              val = val * 10 + (int)(*p - '0');
              if (val > 255)
                return make_pair(match_failure, match_none);;
            }
          break;
        case state_dot:
          if (!isdigit((int)*p))
            return make_pair(match_failure, match_none);;

          val = (int)(*p - '0');
          octets++;
          state = state_digit;
          break;
        case state_slash:
          if (!isdigit((int)*p))
            return make_pair(match_failure, match_none);;

          plen = (int)(*p - '0');
          state = state_plen;
          break;
        case state_plen:
          if (!isdigit((int)*p))
            return make_pair(match_failure, match_none);;

          plen = plen * 10 + (int)(*p - '0');
          if (plen > 32)
            return make_pair(match_failure, match_none);;

          break;
        }

      p++;
    }

  if (state != state_plen)
    return make_pair(match_success, match_incomplete);

  return make_pair(match_success, match_full);
}

MatchState
CliNodeIPv4Address::cli_match(string& input)
{
  const char *str = input.c_str();
  const char *p = str;
  int val;
  int dots = 0;
  int octets = 0;

  enum {
    state_init,
    state_digit,
    state_dot
  };

  int state = state_init;

  while (*p != '\0')
    {
      switch (state)
        {
        case state_init:
          if (!isdigit((int)*p))
            return make_pair(match_failure, match_none);

          state = state_digit;
          octets++;
          val = (int)(*p - '0');
          break;
        case state_digit:
          if (!isdigit((int)*p) && *p != '.')
            return make_pair(match_failure, match_none);

          if (*p == '.')
            {
              state = state_dot;
              dots++;
              if (dots > 3)
                return make_pair(match_failure, match_none);;
            }
          else
            {
              val = val * 10 + (int)(*p - '0');
              if (val > 255)
                return make_pair(match_failure, match_none);;
            }
          break;
        case state_dot:
          if (!isdigit((int)*p))
            return make_pair(match_failure, match_none);;

          val = (int)(*p - '0');
          octets++;
          state = state_digit;
          break;
        }

      p++;
    }

  if (octets != 4)
    return make_pair(match_success, match_incomplete);

  return make_pair(match_success, match_full);
}

MatchState
CliNodeIPv6Prefix::cli_match(string& input)
{
  struct in6_addr addr;
  int ret;

  ret = inet_pton(AF_INET6, input.c_str(), &addr);
  if (ret == 0)
    return make_pair(match_failure, match_none);;

  return make_pair(match_success, match_full);
}

MatchState
CliNodeIPv6Address::cli_match(string& input)
{
  struct in6_addr addr;
  int ret;

  ret = inet_pton(AF_INET6, input.c_str(), &addr);
  if (ret == 0)
    return make_pair(match_failure, match_none);;

  return make_pair(match_success, match_full);
}
