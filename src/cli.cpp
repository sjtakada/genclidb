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
Cli::signal_init()
{
  signal(SIGWINCH, &cli_handle_sigwinch);
}

void
Cli::terminal_init()
{
  ioctl(0, TIOCGWINSZ, &ws_);
}

void
Cli::init()
{
  // signal init
  signal_init();

  // Terminal init.
  terminal_init();

  // CLI mode init.
  mode_read((char *)"../cli.json/quagga.cli_mode.json");

  // Read CLI definitions.
  load_cli_json_all((char *)"../cli.json");
  // exit(0);
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
}

void
Cli::loop()
{
  while (rl_.gets())
    rl_.execute();
}


// Utility to read CLI JSON file.
int
Cli::json_read(char *filename, Json::Value& root)
{
  Json::Reader reader;
  ifstream f(filename);
  string str;
  bool ret;

  f.seekg(0, ios::end);
  str.reserve(f.tellg());
  f.seekg(0, ios::beg);

  str.assign((istreambuf_iterator<char>(f)),
             (istreambuf_iterator<char>()));

  ret = reader.parse(str, root);
  if (!ret)
    cout << "Failed to read CLI JSON file: " << filename << endl;

  return ret;
}

void
Cli::parse_defun(Json::Value& tokens, Json::Value& command)
{
  Json::Value defun = command["defun"];
  Json::Value modes = command["mode"];
  Json::Value action = command["action"];

  if (!defun.isNull() && !modes.isNull())
    for (Json::Value::iterator it = modes.begin(); it != modes.end(); ++it)
      {
        Json::Value mode = (*it);

        CliTree *tree = tree_[mode.asCString()];
        if (tree)
          tree->build_command(defun, tokens, action);
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
      const char *mode = key.asCString();

      if (!value.isNull())
        if (!value["prompt"].isNull())
          prompt = value["prompt"].asString();

      tree = new CliTree(mode, prompt, parent);
      tree_[mode] = tree;

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

void
Cli::mode_read(char *filename)
{
  Json::Value root;

  json_read(filename, root);
  mode_ = mode_traverse(root, NULL);
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
