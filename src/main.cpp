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
#include <getopt.h>
#include <string.h>
#include "cli.hpp"

#define CLI_VERSION "1.0"

static char copyright[] = "Copyright 2014 Toshiaki Takada";

struct option longopts[] =
{
  { "cli_mode", required_argument, NULL, 'm'},
  { "cli_json", required_argument, NULL, 'j'},
  { "null_key", required_argument, NULL, 'n'},
  { "debug",    no_argument,       NULL, 'd'},
  { "help",     no_argument,       NULL, 'h'},
  { "version",  no_argument,       NULL, 'v'},
  { 0 }
};

static void
print_help(char *progname)
{
  cout << "Usage: " << progname << " [OPTION...]" << endl << endl;
  cout << "-m, --cli-mode  Set CLI mode file" << endl;
  cout << "-j, --cli-json  Set CLI JSON directory" << endl;
  cout << "-n, --null-key  Set string for Null as a key" << endl;
  cout << "-d, --debug     Runs in debug mode" << endl;
  cout << "-h, --help      Display this help and exit" << endl;
  cout << "-v, --version   Print program version" << endl;
  cout << endl;

  exit(0);
}

static void
print_version(char *progname)
{
  cout << progname << " version " << CLI_VERSION << endl;
  cout << copyright << endl;
  cout << endl;

  exit(0);
}

int
main(int argc, char **argv)
{
  Cli *cli;
  char *progname;
  char *p;
  bool debug = false;
  char *cli_mode = NULL;
  char *cli_json = NULL;
  char *null_key = NULL;

  progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);

  // Command line options
  while (1)
    {
      int opt;

      opt = getopt_long(argc, argv, "m:j:n:dhv", longopts, 0);
      if (opt == EOF)
        break;

      switch (opt)
        {
        case 0:
          break;
        case 'm':
          cli_mode = optarg;
          break;
        case 'j':
          cli_json = optarg;
          break;
        case 'n':
          null_key = optarg;
          break;
        case 'd':
          debug = true;
          break;
        case 'h':
          print_help(progname);
          break;
        case 'v':
          print_version(progname);
          break;
        default:
          break;
        }
    }

  // Create Cli instance.
  cli = Cli::instance();
  cli->set_debug(debug);
  if (cli_mode)
    cli->set_cli_mode_file(cli_mode);
  if (cli_json)
    cli->set_cli_json_dir(cli_json);
  if (null_key)
    cli->set_null_key(null_key);

  // Init CLI.
  if (!cli->init())
    {
      cout << "CLI Init failure" << endl;
      return 1;
    }

    cli->loop();

  return 0;
}
