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

void
CliReadline::split_token(char *line, vector<string *>& tokens)
{
  char *p = line;
  char *q;

  while (*p != '\0')
    {
      // Skip whitespaces.
      while (isspace(*p))
        p++;

      if (*p == '\0')
        return;

      q = p;
      while (!isspace(*q) && *q != '\0')
        q++;

      if (q != p)
        tokens.push_back(new string(p, q - p));
      // TODO have to delete string

      p = q;
    }
}

int
CliReadline::match_token(string *input, CliNode *node, CliNodeVector& matched)
{
  CliNode *current;

  for (CliNodeVector::iterator it = node->next_.begin();
       it != node->next_.end(); ++it)
    {
      current = (*it);

      if (current->cli_match(input))
        matched.push_back(current);
    }

  return matched.size();
}

int
CliReadline::parse(CliNode *node, CliNodeVector& matched)
{
  vector<string *> input_tokens;

  split_token(rl_line_buffer, input_tokens);

  for (vector<string *>::size_type i = 0; i < input_tokens.size(); ++i)
    {
      string *input = input_tokens[i];
      size_t count;

      matched.clear();

      count = match_token(input, node, matched);
      if (count == 0)
        return CliReadline::match_unrecognized;
      // If this ambiguity is not the last token, we'll give an error.
      else if (count > 1 && input_tokens.size() - 1 != i)
        return CliReadline::match_ambiguous;

      // Continue next node.
      node = matched[0];
    }

  // Cleanup input tokens.

  return CliReadline::match_success;
}

int
CliReadline::describe()
{
  // current mode.
  CliTree *tree = cli_->current_tree();
  CliNode *node = tree->top_;
  CliNode *candidate;
  CliNodeVector matched;
  int ret;
  bool is_cmd_ = false;

  cout << "?" << endl;

  // No input.
  if (rl_end == 0 || strspn(rl_line_buffer, " \t\n") == rl_end)
    {
      for (CliNodeVector::iterator it = node->next_.begin();
           it != node->next_.end(); ++it)
          matched.push_back(*it);
    }
  else
    {
      ret = parse(tree->top_, matched);
      if (ret == CliReadline::match_unrecognized)
        {
          cout << "% Unrecognized command" << endl;
          return 0;
        }
      else if (ret == CliReadline::match_ambiguous)
        {
          cout << "% Ambiguous command" << endl;
          return 0;
        }

      if (isspace(rl_line_buffer[rl_end - 1]))
        {
          node = matched[0];
          matched.clear();

          for (CliNodeVector::iterator it = node->next_.begin();
               it != node->next_.end(); ++it)
            matched.push_back(*it);

          if (node->cmd_)
            is_cmd_ = true;
        }
    }

  size_t max_len = 0;
  for (CliNodeVector::iterator it = matched.begin();
       it != matched.end(); ++it)
    {
      candidate = (*it);
      size_t len = strlen(candidate->cli_token());
      if (max_len < len)
        max_len = len;
    }

  for (CliNodeVector::iterator it = matched.begin();
       it != matched.end(); ++it)
    {
      candidate = (*it);
      cout << "  " << left << setw(max_len + 2)
           << candidate->cli_token() << candidate->help() << endl;
    }

  // TODO: need to consider more.
  if (is_cmd_)
    cout << "  <cr>" << endl;

  cout << prompt();
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
  CliTree *tree = cli_->current_tree();
  CliNodeVector matched;
  int ret;

  // No input. 
  if (rl_end == 0)
    {
      cout << endl;
      return NULL;
    }

  if (state == 0)
    {
      matched_index_ = 0;

      ret = parse(tree->top_, matched);
      if (matched.size() > 0)
        {
          if (matched.size() == 1 &&
              isspace(rl_line_buffer[rl_end - 1]))
            {
              CliNode *node = matched[0];
              matched.clear();

              for (CliNodeVector::iterator it = node->next_.begin();
                   it != node->next_.end(); ++it)
                {
                  matched.push_back(*it);
                }
            }

          int i = 0;

          matched_strvec_ =
            (char **)calloc(matched.size() + 1, sizeof(char *));
          for (CliNodeVector::iterator it = matched.begin();
               it != matched.end(); ++it)
            {
              CliNode *node = (*it);
              if (node->type_ == CliTree::keyword)
                matched_strvec_[i++] = strdup(node->cli_token());
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
  buf_ = readline(prompt());

  // Add history.
  if (buf_ && buf_[0] != '\0')
    add_history(buf_);

  return buf_;
}

char *
CliReadline::prompt()
{
  return (char *)"Router> ";
}

