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
#include <iostream>
#include <iomanip>

#include <readline/readline.h>
#include <readline/history.h>

#include "project.hpp"
#include "cli_tree.hpp"
#include "cli_readline.hpp"
#include "cli.hpp"

const boost::regex CliReadline::re_white_space("^([[:space:]]*)");
const boost::regex CliReadline::re_white_space_only("^([[:space:]]*)$");
const boost::regex CliReadline::re_command_string("^[^[:space:]]+");

bool
CliNodeMatchStateCriterion(CliNodeMatchStatePair n, CliNodeMatchStatePair m)
{
  return (n.second).second < (m.second).second;
}

size_t
CliReadline::match_token(string& input, CliNode *curr,
                         CliNodeMatchStateVector& state_vec)
{
  CliNode *next;
  CliNodeMatchStateVector matched_vec;

  for (CliNodeVector::iterator it = curr->next_.begin();
       it != curr->next_.end(); ++it)
    {
      next = (*it);

      MatchState state;

      state = next->cli_match(input);
      if (state.first == match_success)
        {
          CliNodeMatchStatePair p = make_pair(next, state);
          matched_vec.push_back(p);
        }
    }

  // Sort by type of match.
  if (matched_vec.size() > 1)
    sort(matched_vec.begin(), matched_vec.end(), CliNodeMatchStateCriterion);

  state_vec = matched_vec;

  return matched_vec.size();
}

bool
CliReadline::get_token(string& str, string& token)
{
  boost::smatch m;

  if (boost::regex_search(str, m, re_command_string))
    {
      token = m[0];
      str = m.suffix();

      return true;
    }

  return false;
}

bool
CliReadline::skip_spaces(string& str)
{
  boost::smatch m;

  if (boost::regex_search(str, m, re_white_space))
    {
      str = m.suffix();
      return true;
    }

  return false;
}

void
CliReadline::fill_matched_vec(CliNode *node,
                              CliNodeMatchStateVector& matched_vec)
{
  for (CliNodeVector::iterator it = node->next_.begin();
       it != node->next_.end(); ++it)
    {
      MatchState state = make_pair(match_success, match_partial);
      CliNodeMatchStatePair p = make_pair(*it, state);
      matched_vec.push_back(p);
    }
}

void
CliReadline::filter_matched(CliNodeMatchStateVector& matched_vec)
{
  CliNodeMatchStateVector vec;
  MatchState state;

  if (matched_vec.size() <= 1)
    return;

  state = matched_vec[0].second;

  for (CliNodeMatchStateVector::iterator it = matched_vec.begin();
       it != matched_vec.end(); )
    {
      if (it->second.second != state.second)
        it = matched_vec.erase(it);
      else
        ++it;
    }
}

// Return true if command can be completed.
bool
CliReadline::parse(string& line, CliNode *curr,
                   CliNodeMatchStateVector& matched_vec)
{
  boost::smatch m;
  string token;

  matched_vec.clear();
  if (!skip_spaces(line))
    return curr->cmd_;

  fill_matched_vec(curr, matched_vec);

  if (!get_token(line, token))
    return curr->cmd_;

  match_token(token, curr, matched_vec);
  if (line.begin() != line.end())
    {
      filter_matched(matched_vec);

      if (matched_vec.size() == 1)
        return parse(line, matched_vec[0].first, matched_vec);
    }

  if (matched_vec.size() == 1)
    curr = matched_vec[0].first;

  return curr->cmd_;
}

enum ExecResult
CliReadline::parse_execute(string& line, CliNode *curr,
                           CliNodeTokenVector& node_token_vec)
{
  boost::smatch m;
  string token;
  CliNodeMatchStateVector matched_vec;

  do {
    if (!skip_spaces(line))
      break;

    if (!get_token(line, token))
      break;

    fill_matched_vec(curr, matched_vec);
    match_token(token, curr, matched_vec);
    filter_matched(matched_vec);

    if (matched_vec.size() == 0)
      return exec_unrecognized;
    else if (matched_vec.size() > 1)
      return exec_ambiguous;

    CliNodeTokenPair p = make_pair(matched_vec[0].first, new string(token));
    node_token_vec.push_back(p);

    // Candidate is only one at this point.
    if (line.begin() != line.end())
      return parse_execute(line, matched_vec[0].first, node_token_vec);

    if (matched_vec[0].second.second == match_incomplete)
      return exec_incomplete;

    if (!matched_vec[0].first->cmd_)
      return exec_incomplete;

    curr = matched_vec[0].first;

  } while (0);

  if (curr->cmd_)
    return exec_complete;

  return exec_unrecognized;
}

void
CliReadline::describe_line(CliNode *node, size_t max_len_token)
{
  size_t max_len_help;
  string cli_token(node->cli_token());
  string help(node->help());
  string substr;

  // No enough space to show help, just print without folding.
  if (cli_->ws_.ws_col < max_len_token + 5)
    {
      cout << "  " << left << setw(max_len_token + 2)
           << cli_token << node->help() << endl;
      return;
    }

  max_len_help = cli_->ws_.ws_col - (max_len_token + 5);

  while (help.size() > max_len_help)
    {
      const char *s = help.c_str();
      const char *p = s + max_len_help;

      while (*p != ' ' && p != s)
        p--;

      if (p == s)
        break;

      size_t length = (size_t)(p - s);
      substr = help.substr(0, length);

      cout << "  " << left << setw(max_len_token + 2)
           << cli_token << substr << endl;

      help = help.substr(length + 1);

      cli_token = " ";
    }

  cout << "  " << left << setw(max_len_token + 2)
       << cli_token << help << endl;
}

int
CliReadline::describe()
{
  // current mode.
  CliTree *tree = cli_->current_mode();
  CliNode *candidate;
  CliNodeMatchStateVector matched_vec;
  string line(" ");
  bool is_cmd = false;

  line += rl_line_buffer;

  cout << "?" << endl;

  is_cmd = parse(line, tree->top_, matched_vec);
  if (!is_cmd && matched_vec.size() == 0)
    {
      cout << "% Unrecognized command" << endl << endl;
    }
  else
    {
      size_t max_len = 0;
      for (CliNodeMatchStateVector::iterator it = matched_vec.begin();
           it != matched_vec.end(); ++it)
        {
          candidate = it->first;
          size_t len = candidate->cli_token().size();
          if (max_len < len)
            max_len = len;
        }

      for (CliNodeMatchStateVector::iterator it = matched_vec.begin();
           it != matched_vec.end(); ++it)
        describe_line(it->first, max_len);

      if (is_cmd)
        cout << "  <cr>" << endl;
    }

  cout << cli_->prompt();
  cout << rl_line_buffer;

  return 0;
}

int
readline_describe(int, int)
{
  return Cli::readline().describe();
}

static char *
readline_completion_dummy(const char *, int)
{
  return NULL;
}

char *
CliReadline::completion_matches(const char *text, int state)
{
  CliTree *tree = cli_->current_mode();
  CliNodeMatchStateVector matched_vec;
  string line(" ");
  bool is_cmd = false;

  line += rl_line_buffer;

  // No input. 
  if (rl_end == 0)
    {
      cout << endl << endl;
      cout << cli_->prompt();
      return NULL;
    }

  if (state == 0)
    {
      matched_strvec_ = NULL;
      matched_index_ = 0;

      is_cmd = parse(line, tree->top_, matched_vec);
      if (matched_vec.size() == 0)
        {
          cout << endl;
          if (!is_cmd)
            cout << "% Unrecognized command" << endl << endl;
          cout << cli_->prompt();
          cout << rl_line_buffer;
        }
      else
        {
          int i = 0;

          matched_strvec_ =
            (char **)calloc(matched_vec.size() + 1, sizeof(char *));

          for (CliNodeMatchStateVector::iterator it = matched_vec.begin();
               it != matched_vec.end(); ++it)
            {
              CliNode *node = it->first;
              if (node->type_ == CliTree::keyword)
                matched_strvec_[i++] = strdup(node->cli_token().c_str());
            }
        }
    }

  if (matched_strvec_ && matched_strvec_[matched_index_])
    return matched_strvec_[matched_index_++];

  return NULL;
}

char *
readline_completion_matches(const char *text, int state)
{
  return Cli::readline().completion_matches(text, state);
}

char **
CliReadline::completion(const char *text, int start, int end)
{
  return rl_completion_matches(text, readline_completion_matches);
}

char **
readline_completion(const char *text, int start, int end)
{
  return Cli::readline().completion(text, start, end);
}

void
CliReadline::init(Cli *cli)
{
  cli_ = cli;

  rl_bind_key('?', readline_describe);
  rl_completion_entry_function = readline_completion_dummy;
  rl_attempted_completion_function = readline_completion;
  rl_completion_append_character = '\0';
}

char *
CliReadline::gets()
{
  // Clear readline read buffer first.
  if (buf_)
    {
      free(buf_);
      buf_ = NULL;
    }

  // Read a line.
  buf_ = readline(cli_->prompt());

  // Add history.
  if (buf_ && buf_[0] != '\0')
    add_history(buf_);

  return buf_;
}

bool
CliReadline::execute()
{
  // current mode.
  CliTree *tree = cli_->current_mode();
  CliNodeTokenVector node_token_vec;
  string line(" ");
  boost::smatch m;

  line += rl_line_buffer;

  if (!boost::regex_search(line, m, re_white_space_only))
    {
      enum ExecResult
        result = parse_execute(line, tree->top_, node_token_vec);

      switch (result)
        {
        case exec_complete:
          {
            for (CliNodeTokenVector::iterator it = node_token_vec.begin();
                 it != node_token_vec.end(); ++it)
              {
                cout << "token: " << it->first->cli_token() << " "
                     << "input: " << *it->second << endl;
              }

            CliNode *node = node_token_vec.back().first;
            if (!node->next_mode_.empty())
              cli_->mode_set(node->next_mode_);
          }
          break;
        case exec_incomplete:
          cout << "% Incomplete command" << endl << endl;
          break;
        case exec_ambiguous:
          cout << "% Ambiguous command" << endl << endl;
          break;
        case exec_unrecognized:
          cout << "% Unrecognized command" << endl << endl;
          break;
        }

      // Cleanup input token strings.
      for (CliNodeTokenVector::iterator it = node_token_vec.begin();
           it != node_token_vec.end(); ++it)
        delete it->second;
    }

  return true;
}


