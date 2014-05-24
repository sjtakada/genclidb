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
#ifndef _CLI_ACTION_HPP_
#define _CLI_ACTION_HPP_

#include <map>
#include <string>

#include "project.hpp"

class Cli;

// e.g., "ipv4_addr" => "IPV4-ADDR:1.0" -- HTTP param to CLI def token
typedef map<string, string> ParamTokenMap;

class CliAction
{
public:
  virtual bool handle(Cli *cli, TokenInputMap& input) = 0;
};

class CliActionHttp: public CliAction
{
public:
  CliActionHttp(Json::Value& http);
  bool handle(Cli *cli, TokenInputMap& input);

private:
  string method_;
  string path_;
  ParamTokenMap param_token_;

  void request(Cli *cli, string& method, string& path, string& json);
  bool get_token(string& str, string& token);
};

class CliActionMode: public CliAction
{
public:
  CliActionMode(Json::Value& mode)
    : CliAction()
  {
    Json::Value name = mode["name"];
    Json::Value params = mode["params"];

    name_ = name.asString();

    if (params.isArray())
      {
        for (Json::Value::iterator it = params.begin();
             it != params.end(); ++it)
          {
            params_.push_back((*it).asString());
          }
      }
  }
  bool handle(Cli *cli, TokenInputMap& input);

private:
  string name_;
  StringVector params_;
};

class CliActionBuiltIn: public CliAction
{
public:
  CliActionBuiltIn(Json::Value& built_in)
    : CliAction()
  {
    if (!built_in["func"].isNull())
      func_ = built_in["func"].asString();
  }
  bool handle(Cli *cli, TokenInputMap& params);

private:
  string func_;
};

#endif /* _CLI_ACTION_HPP_ */
