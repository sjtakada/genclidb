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
#include "json/json.h"

#include "project.hpp"
#include "cli_tree.hpp"
#include "cli_action.hpp"
#include "cli_readline.hpp"
#include "cli_string.hpp"
#include "cli_http.hpp"
#include "cli.hpp"

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

      MatchState state = next->cli_match(input);
      if (state.first == match_success)
        {
          CliNodeMatchStatePair p = make_pair(next, state);
          matched_vec.push_back(p);
        }
    }

  state_vec = matched_vec;

  return matched_vec.size();
}

bool
CliReadline::get_token(string& str, string& token)
{
  size_t len;

  len = command_length(str);
  if (len)
    {
      token = str.substr(0, len);
      str = str.substr(len, string::npos);

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
CliReadline::filter_matched(CliNodeMatchStateVector& matched_vec,
                            MatchFlag limit)
{
  CliNodeMatchStateVector vec;

  for (CliNodeMatchStateVector::iterator it = matched_vec.begin();
       it != matched_vec.end(); ++it)
    {
      if (it->second.second > limit)
        continue;

      if (it->second.second < limit)
        {
          vec.clear();
          limit = it->second.second;
        }

      vec.push_back(*it);
    }

  matched_vec = vec;
}

void
CliReadline::filter_hidden(CliNodeMatchStateVector& matched_vec)
{
  CliNodeMatchStateVector vec;

  for (CliNodeMatchStateVector::iterator it = matched_vec.begin();
       it != matched_vec.end(); ++it)
    if (!it->first->is_hidden())
      vec.push_back(*it);

  matched_vec = vec;
}

void
CliReadline::match_shorter(CliParseState& ps, CliNode *curr, string& token)
{
  unsigned int length = token.size();
  unsigned int offset = 0;
  unsigned int parsed_len = ps.len_ - ps.line_.size();

  while (length >= offset)
    {
      string sub_token = token.substr(0, length - offset);

      fill_matched_vec(curr, ps.matched_vec_);
      filter_hidden(ps.matched_vec_);
      
      match_token(sub_token, curr, ps.matched_vec_);
      filter_hidden(ps.matched_vec_);
      if (ps.matched_vec_.size() > 0)
        break;

      offset++;
    }

  ps.matched_len_ = parsed_len - offset;
}

// Return true if command can be completed.
enum ExecResult
CliReadline::parse(CliParseState& ps, CliNode *curr)
{
  string token;

  do {
    ps.matched_vec_.clear();
    if (!trim_spaces_at_head(ps.line_))
      break;

    fill_matched_vec(curr, ps.matched_vec_);
    filter_hidden(ps.matched_vec_);

    if (!get_token(ps.line_, token))
      break;

    match_token(token, curr, ps.matched_vec_);
    filter_hidden(ps.matched_vec_);

    if (ps.line_.begin() != ps.line_.end())
      {
        filter_matched(ps.matched_vec_, match_partial);

        if (ps.matched_vec_.size() == 0)
          {
            match_shorter(ps, curr, token);
            if (curr->next_.size() == 0)
              ps.matched_len_++;

            return exec_unrecognized;
          }

        if (ps.matched_vec_.size() > 1)
          return exec_ambiguous;

        return parse(ps, ps.matched_vec_[0].first);
      }

    if (ps.matched_vec_.size() == 0)
      {
        match_shorter(ps, curr, token);
        if (curr->next_.size() == 0)
          ps.matched_len_++;

        return exec_unrecognized;
      }

    if (ps.matched_vec_.size() == 1)
      curr = ps.matched_vec_[0].first;

  } while (0);

  if ((ps.is_cmd_ = curr->cmd_))
    return exec_complete;

  return exec_incomplete;
}

enum ExecResult
CliReadline::parse_execute(CliParseStateExecute& ps, CliNode *curr)
{
  string token;

  ps.matched_vec_.clear();

  do {
    if (!trim_spaces_at_head(ps.line_))
      break;

    if (curr->next_.size() == 0)
      {
        if (is_white_space_only(ps.line_))
          break;

        ps.matched_len_ = ps.len_ - ps.line_.size();
        return exec_unrecognized;
      }

    if (!get_token(ps.line_, token))
      break;

    fill_matched_vec(curr, ps.matched_vec_);
    match_token(token, curr, ps.matched_vec_);
    filter_matched(ps.matched_vec_, match_partial);

    if (ps.matched_vec_.size() == 0)
      {
        match_shorter(ps, curr, token);
        return exec_unrecognized;
      }
    else if (ps.matched_vec_.size() > 1)
      return exec_ambiguous;

    CliNodeTokenPair p =
      make_pair(ps.matched_vec_[0].first, new string(token));
    ps.node_token_vec_.push_back(p);

    // Candidate is only one at this point.
    if (ps.line_.begin() != ps.line_.end())
      return parse_execute(ps, ps.matched_vec_[0].first);

    if (ps.matched_vec_[0].second.second == match_incomplete)
      return exec_incomplete;

    if (!ps.matched_vec_[0].first->cmd_)
      return exec_incomplete;

    curr = ps.matched_vec_[0].first;

  } while (0);

  if (curr->cmd_)
    return exec_complete;

  return exec_incomplete;
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
  CliParseState ps(rl_line_buffer);
  enum ExecResult result = exec_complete;

  cout << "?" << endl;

  result = parse(ps, tree->top_);
  if (result == exec_ambiguous)
    cout << "% Ambiguous command" << endl << endl;
  else if (result == exec_unrecognized)
    {
      unsigned int offset = strlen(cli_->prompt()) + ps.matched_len_ + 1;
      cout << right << setw(offset) << "^" << endl;
      cout << "% Invalid input detected at '^' marker." << endl << endl;
    }
  else
    {
      size_t max_len = 0;
      for (CliNodeMatchStateVector::iterator it = ps.matched_vec_.begin();
           it != ps.matched_vec_.end(); ++it)
        {
          candidate = it->first;
          size_t len = candidate->cli_token().size();
          if (max_len < len)
            max_len = len;
        }

      for (CliNodeMatchStateVector::iterator it = ps.matched_vec_.begin();
           it != ps.matched_vec_.end(); ++it)
        {
          describe_line(it->first, max_len);

          CliNode *node = it->first;
          for (StringVector::iterator is = node->candidates_.begin();
               is != node->candidates_.end(); ++is)
            cout << "  " << *is << endl;
        }

      if (ps.is_cmd_)
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
  CliParseState ps(rl_line_buffer);
  enum ExecResult result = exec_complete;

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

      result = parse(ps, tree->top_);
      if (result == exec_unrecognized)
        {
          cout << endl;
          cout << cli_->prompt();
          cout << rl_line_buffer;
        }
      else if (result == exec_ambiguous)
        {
          cout << endl;
          cout << "% Ambiguous command" << endl << endl;
          cout << cli_->prompt();
          cout << rl_line_buffer;
        }
      else
        {
          cout << endl;
          cout << cli_->prompt();
          cout << rl_line_buffer;

          int i = 0;

          matched_strvec_ =
            (char **)calloc(ps.matched_vec_.size() + 1, sizeof(char *));

          for (CliNodeMatchStateVector::iterator it = ps.matched_vec_.begin();
               it != ps.matched_vec_.end(); ++it)
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
  char **matches;
  matches = rl_completion_matches(text, readline_completion_matches);

  return matches;
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
  utils_.init();

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

typedef map<string, bool> KeywordsMap;

void
CliReadline::handle_actions(CliNodeTokenVector& node_token_vec)
{
  ParamsMap input;
  KeywordsMap keywords;

  // Populate mode params first.
  for (ParamsMap::iterator it = cli_->params_.begin();
       it != cli_->params_.end(); ++it)
    input[it->first] = it->second;

  if (cli_->is_debug())
    cout << "> CLI parser" << endl;

  // Populate params and keywords.
  for (CliNodeTokenVector::iterator it = node_token_vec.begin();
       it != node_token_vec.end(); ++it)
    {
      CliNode *node = it->first;

      if (cli_->is_debug())
        cout << ">> input '" << *it->second << "' matches " 
             << "token '" << node->cli_token() << "'" << endl;

      if (node->type_ == CliTree::keyword)
        {
          keywords[node->def_token()] = true;
          CliNodeKeyword *knode = (CliNodeKeyword *)node;
          if (!knode->enum_key().empty())
            input[knode->enum_key()] = knode->cli_token();
        }
      else
        {
          input[node->def_token()] = node->format_param(*it->second);
        }
    }

  // Command matched last node.
  CliNode *node = node_token_vec.back().first;

  // Pre-process parameter binding.
  for (CondBindPairVector::iterator it = node->bind_.begin();
       it != node->bind_.end(); ++it)
    {
      const char *cond = it->first.c_str();

      // Always bind if it is TRUE.
      if (cond[0] == '\0')
        utils_.bind_interpreter(it->second, input);
      // If the parameter is NOT present.
      else if (cond[0] == '!')
        {
          ParamsMap::iterator is = input.find(&cond[1]);
          if (is == input.end())
            utils_.bind_interpreter(it->second, input);
        }
      // If the parameter is present.
      else
        {
          ParamsMap::iterator is = input.find(cond);
          if (is != input.end())
            utils_.bind_interpreter(it->second, input);
        }
    }

  // Special NULL binding.
  input["NULL"] = cli_->null_key();

  // Dump all inputs.
  if (cli_->is_debug())
    {
      cout << "> Params Binding" << endl;
      for (ParamsMap::iterator it = input.begin();
           it != input.end(); ++it)
        cout << ">> params[" << it->first << "] = " << it->second << endl;
    }

  if (node->actions_.size() == 0)
    {
      if (cli_->is_debug())
        cout << "No action defined" << endl;
    }
  else
    {
      // Dispatch action to appropriate handler.
      for (CliActionVector::iterator it = node->actions_.begin();
           it != node->actions_.end(); ++it)
        {
          string cond = it->first;
          CliAction *action = it->second;

          // This is default action.
          if (cond == "*")
            action->handle(cli_, input);
          else
            {
              ParamsMap::iterator is = input.find(cond);
              if (is != input.end())
                action->handle(cli_, input);

              KeywordsMap::iterator ik = keywords.find(cond);
              if (ik != keywords.end())
                action->handle(cli_, input);
            }
        }
    }
}

enum ExecResult
CliReadline::execute_parent(CliParseStateExecute& ps, CliTree *mode)
{
  ps.reset();

  enum ExecResult result = parse_execute(ps, mode->top_);

  if (result == exec_complete)
    {
      handle_actions(ps.node_token_vec_);

      cli_->mode_set(mode);
    }
  else if (mode->parent())
    {
      result = execute_parent(ps, mode->parent());
    }

  return result;
}

bool
CliReadline::execute()
{
  CliTree *mode = cli_->current_mode();
  CliParseStateExecute ps(rl_line_buffer);

  if (!is_white_space_only(ps.line_))
    {
      enum ExecResult
        result = parse_execute(ps, mode->top_);

      switch (result)
        {
        case exec_complete:
          handle_actions(ps.node_token_vec_);
          break;
        case exec_incomplete:
          cout << "% Incomplete command" << endl << endl;
          break;
        case exec_ambiguous:
          cout << "% Ambiguous command" << endl << endl;
          break;
        case exec_unrecognized:
          {
            u_int16_t matched_len = ps.matched_len_;
            if (mode->parent())
              result = execute_parent(ps, mode->parent());

            if (result != exec_complete)
              {
                unsigned int offset =
                  strlen(cli_->prompt()) + matched_len + 1;
                cout << right << setw(offset) << "^" << endl;
                cout << "% Invalid input detected at '^' marker."
                     << endl << endl;
              }
          }
          break;
        }
    }

  return true;
}

