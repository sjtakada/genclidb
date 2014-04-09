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
Cli::init()
{
  // signal init

  // CLI mode init.
  cli_mode_read((char *)"cli.json/quagga.cli_mode.json");

  // Read CLI definitions.
  cli_read((char *)"cli.json/ospf_vty.cli.json");

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


// Read CLI JSON file and build CLI tree.
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
    cout << "Failed to read cli.json";

  return ret;
}

void
Cli::cli_parse_command(Json::Value& tokens, Json::Value& command)
{
  Json::Value defun = command["defun"];
  Json::Value modes = command["mode"];

  if (!defun.isNull() && !modes.isNull())
    for (Json::Value::iterator it = modes.begin(); it != modes.end(); ++it)
      {
        Json::Value mode = (*it);

        CliTree *tree = tree_[mode.asCString()];
        if (tree)
          tree->parse_defun(defun, tokens);
      }
}

void
Cli::cli_parse_command_all(Json::Value& tokens, Json::Value& commands)
{
  if (!tokens.isNull() && !commands.isNull())
    for (Json::Value::iterator it = commands.begin();
         it != commands.end(); ++it)
      {
        cli_parse_command(tokens, (*it));
      }
}

void
Cli::cli_read(char *filename)
{
  Json::Value root;

  // Read a CLI JSON file.
  json_read(filename, root);

  // Iterate all commands definition.
  for (Json::Value::iterator it = root.begin(); it != root.end(); ++it)
    {
      Json::Value attr = (*it);

      if (!attr.isNull())
        cli_parse_command_all(attr["token"], attr["command"]);
    }
}

void
Cli::cli_mode_traverse(Json::Value& current, CliTree *parent)
{
  for (Json::Value::iterator it = current.begin(); it != current.end(); ++it)
    {
      Json::Value key = it.key();
      Json::Value value = (*it);
      const char *mode = key.asCString();

      CliTree *tree = new CliTree(mode, parent);
      tree_[mode] = tree;

      if (!value["children"].isNull())
        cli_mode_traverse(value["children"], tree);
    }
}

void
Cli::cli_mode_read(char *filename)
{
  Json::Value root;

  json_read(filename, root);

  cli_mode_traverse(root, NULL);
}

