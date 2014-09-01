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
#include <signal.h>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <streambuf>

#include "json/json.h"

#include "project.hpp"
#include "cli_tree.hpp"
#include "cli_readline.hpp"
#include "cli.hpp"

// Singleton instance.
Cli *Cli::instance_ = NULL;

Cli *
Cli::instance()
{
  if (instance_ == NULL)
    instance_ = new Cli;

  return instance_;
}

void
cli_handle_sigwinch(int sig)
{
  Cli::instance()->terminal_init();
}

void
cli_handle_sigint (int sig)
{
  Cli::instance()->end();
  Cli::instance()->start_over();
}

void
cli_handle_sigtstp (int sig)
{
  Cli::instance()->end();
  Cli::instance()->start_over();
}

void
Cli::signal_init()
{
  signal(SIGINT, &cli_handle_sigint);
  signal(SIGTSTP, &cli_handle_sigtstp);
  signal(SIGWINCH, &cli_handle_sigwinch);
}

void
Cli::terminal_init()
{
  ioctl(0, TIOCGWINSZ, &ws_);
}

bool
Cli::init()
{
  // Signal init
  signal_init();

  // Terminal init.
  terminal_init();

  // CLI mode init.
  if (!mode_read((char *)cli_mode_file().c_str()))
    return false;

  // Built-In init.
  built_in_.init(this);

  // Read CLI definitions.
  load_cli_json_all((char *)cli_json_dir().c_str());

  // Sort CLI trees.
  for (ModeTreeMap::iterator it = tree_.begin(); it != tree_.end(); ++it)
    {
      CliTree *tree = it->second;
      if (tree)
        tree->top_->sort_recursive();
    }

  // readline init
  rl_.init(this);

  // banner

  return true;
}

void
Cli::start_over()
{
  cout << endl;
  cout << prompt();
}

void
Cli::exit()
{
  if (mode_->exit_to_finish())
    {
      exit_ = true;
      cout << endl << "CLI Session terminated" << endl;
    }
  else if (mode_->exit_to_end())
    {
      end();
    }
  else
    {
      mode_up(1);
    }

  rl_.clear();
}

// Supposed to fallback to privilege mode.
void
Cli::end()
{
  if (!privileged_mode_.empty())
    mode_set(privileged_mode_);
}

void
Cli::loop()
{
  while (!exit_)
    {
      if (rl_.gets())
        rl_.execute();
      else 
        exit();
    }
}


// Utility to read CLI JSON file.
int
Cli::json_read(char *filename, Json::Value& root)
{
  Json::Reader reader;
  ifstream f;
  string str;
  bool ret;

  f.exceptions(std::ifstream::failbit);
  try
    {
      f.open(filename);
    }
  catch (std::ifstream::failure e)
    {
      if (is_debug())
        cout << "File cannot open: " << filename << endl;

      return 0;
    }

  f.seekg(0, ios::end);
  str.reserve(f.tellg());
  f.seekg(0, ios::beg);

  str.assign((istreambuf_iterator<char>(f)),
             (istreambuf_iterator<char>()));

  ret = reader.parse(str, root);
  if (!ret)
    cout << "Failed to parse JSON file: " << filename << endl;

  return ret;
}

void
Cli::parse_defun(Json::Value& tokens, Json::Value& command)
{
  Json::Value modes = command["mode"];

  if (!modes.isNull())
    for (Json::Value::iterator it = modes.begin(); it != modes.end(); ++it)
      {
        Json::Value mode = (*it);

        CliTree *tree = tree_[mode.asCString()];
        if (tree)
          tree->build_command(tokens, command);
      }
}

void
Cli::parse_defun_all(Json::Value& tokens, Json::Value& commands)
{
  if (!tokens.isNull() && !commands.isNull())
    for (Json::Value::iterator it = commands.begin();
         it != commands.end(); ++it)
      parse_defun(tokens, (*it));
}

bool
Cli::load_cli_json_all(char *dirname)
{
  DIR *dirp;
  struct dirent entry;
  struct dirent *result;
  const char *suffix = ".cli.json";
  size_t suffix_len = strlen(suffix);

  dirp = opendir(dirname);
  if (dirp == NULL)
    {
      cout << "Error: cannot read directory " << dirname << endl;
      return false;
    }

  while (!readdir_r(dirp, &entry, &result))
    {
      if (result == NULL)
        break;

      size_t len = strlen(entry.d_name);
      if (len > suffix_len)
        if ((strncmp((const char *)&entry.d_name[len - suffix_len],
                     suffix, suffix_len)) == 0)
          {
            string file(dirname);
            file += "/";
            file += entry.d_name;

            if (is_debug())
              cout << "Loading CLI JSON " << file << endl; // debug
            load_cli_json((char *)file.c_str());
          }
    }

  closedir(dirp);

  return true;
}

void
Cli::load_cli_json(char *filename)
{
  Json::Value root;

  // Read a CLI JSON file.
  json_read(filename, root);

  // Iterate all commands definition.
  for (Json::Value::iterator it = root.begin(); it != root.end(); ++it)
    {
      Json::Value attr = (*it);

      if (!attr.isNull())
        parse_defun_all(attr["token"], attr["command"]);
    }
}

// Return root mode.
CliTree *
Cli::mode_traverse(Json::Value& current, CliTree *parent)
{
  CliTree *tree = NULL;

  for (Json::Value::iterator it = current.begin(); it != current.end(); ++it)
    {
      Json::Value key = it.key();
      Json::Value value = (*it);
      string prompt;
      bool exit_to_finish = false;
      bool exit_to_end = false;
      const char *mode_name = key.asCString();

      if (!value.isNull())
        {
          if (!value["prompt"].isNull())
            prompt = value["prompt"].asString();

          if (!value["exit-to-finish"].isNull())
            exit_to_finish = value["exit-to-finish"].asBool();

          if (!value["exit-to-end"].isNull())
            exit_to_end = value["exit-to-end"].asBool();

          if (!value["privileged-mode"].isNull())
            privileged_mode_ = mode_name;
        }

      tree = new CliTree(mode_name, prompt, parent,
                         exit_to_finish, exit_to_end);
      tree_[mode_name] = tree;

      if (!value["children"].isNull())
        mode_traverse(value["children"], tree);
    }

  return tree;
}

bool
Cli::mode_set(string& mode_str)
{
  CliTree *mode = tree_[mode_str];

  if (mode)
    {
      mode_ = mode;
      return true;
    }

  return false;
}

bool
Cli::mode_set(CliTree *mode)
{
  mode_ = mode;
  return true;
}

bool
Cli::mode_up(unsigned int up)
{
  CliTree *mode = current_mode();

  // XXX/ currently just go 1 up.
  if (up)
    if (mode->parent())
      {
        mode_ = mode->parent();
        return true;
      }

  return false;
}

int
Cli::mode_read(char *filename)
{
  Json::Value root;
  int ret;

  ret = json_read(filename, root);
  if (ret)
    mode_ = mode_traverse(root, NULL);

  return ret;
}

const char *
Cli::prompt()
{
  string str(hostname_);
  CliTree *tree = current_mode();

  str += tree->prompt();
  str += " ";

  return str.c_str();
}
