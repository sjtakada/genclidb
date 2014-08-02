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
#include <algorithm>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Exception.hpp>
#include "json/json.h"

#include "cli.hpp"
#include "cli_action.hpp"

using namespace curlpp;

CliActionHttp::CliActionHttp(Json::Value& http)
  : CliAction()
{
  method_ = http["method"].asString();
  path_ = http["path"].asString();
  format_ = http["format"].asString();

  Json::Value params = http["params"];

  if (!params.isNull())
    for (Json::Value::iterator it = params.begin();
         it != params.end(); ++it)
      param_token_[it.key().asString()] = *it;
}

bool
CliActionHttp::get_token(string& str, string& token)
{
  const char *p = str.c_str();
  size_t pos;

  // skip "/".
  pos = strspn(p, "/");
  if (pos > 0)
    str = str.substr(pos, string::npos);

  // stirng is empty.
  if (str.empty())
    return false;

  p = str.c_str();
  pos = strcspn(p, "/");
  token = str.substr(0, pos);

  str = str.substr(pos, string::npos);

  return true;
}

bool
CliActionHttp::handle(Cli *cli, ParamsMap& input)
{
  Json::Value json_params;
  Json::FastWriter writer;
  Json::Value null_value;
  string str(path_);
  string path;
  string token;
  const char *delim = "";

  if (cli->is_debug())
    {
      cout << "method: " << method_ << endl;
      cout << "path: " << path_ << endl;
    }

  for (ParamTokenMap::iterator it = param_token_.begin();
       it != param_token_.end(); ++it)
    {
      string key = it->first;
      ParamsMap::iterator is;

      is = input.find(key);
      if (is != input.end())
        key = is->second;

      replace(key.begin(), key.end(), '-', '_');

      if (cli->is_debug())
        cout << "param_token_[" << key << "] = " << it->second << endl;

      if (it->second.isNull())
        json_params[key] = null_value;
      else if (it->second.isBool())
        json_params[key] = it->second.asBool();
      else if (it->second.isUInt64())
        json_params[key] = it->second.asUInt64();
      else if (it->second.isInt64())
        json_params[key] = it->second.asInt64();
      else if (it->second.isString())
        {
          if (!input[it->second.asString()].empty())
            json_params[key] = input[it->second.asString()];
          else
            json_params[key] = it->second;
        }
    }

  // Generate path with parameter.
  while (get_token(str, token))
    {
      path += delim;

      if (token.c_str()[0] == ':')
        path += input[&token.c_str()[1]];
      else
        path += token;

      delim = "/";
    }

  string json_str = writer.write(json_params);

  if (cli->is_debug())
    cout << "json: " << json_str << endl;

  request(cli, method_, path, json_str);

  return true;
}

void
CliActionHttp::request(Cli *cli, string& method, string& path, string& json)
{
  if (method != "GET"
      && method != "POST"
      && method != "PUT"
      && method != "DELETE")
    return;

  string url("http://localhost");
  string api_prefix("/zebra/api");

  url += api_prefix;
  if (!path.empty())
    url += "/" + path;

  if (format_.empty())
    url += ".json";
  else
    url += "." + format_;

  try
    {
      Cleanup myCleanup;
      Easy req;
      stringstream result;

      req.setOpt(new options::Url(url));
      if (cli->is_debug())
        req.setOpt(new options::Verbose(true));

      list<string> header;
      header.push_back("Content-type: application/json");

      req.setOpt(new options::HttpHeader(header));
      req.setOpt(new options::CustomRequest(method));
      req.setOpt(new options::WriteStream(&result));

      if (method != "GET")
        {
          req.setOpt(new options::PostFields(json));
          req.setOpt(new options::PostFieldSize(json.size()));
        }

      req.perform();

      int status = infos::ResponseCode::get(req);
      switch (status / 100)
        {
        case 1:
        case 2:
          if (format_ == "cli")
            cout << result.str() << endl;
          break;
        case 3:
        case 4:
        case 5:
          cout << "HTTP Error: " << status << endl;
          break;
        }

      cli->set_result(result);
    }
  catch (curlpp::RuntimeError& e)
    {
      if (cli->is_debug())
        cout << e.what() << std::endl;
    }
  catch (curlpp::LogicError& e)
    {
      if (cli->is_debug())
        cout << e.what() << std::endl;
    }

  //  cout << endl;
}


CliActionMode::CliActionMode(Json::Value& mode)
  : CliAction()
{
  Json::Value name = mode["name"];
  Json::Value up = mode["up"];
  Json::Value params = mode["params"];

  if (!name.isNull())
    name_ = name.asString();
  if (!up.isNull())
    up_ = up.asUInt();

  if (!params.isNull())
    for (Json::Value::iterator it = params.begin();
         it != params.end(); ++it)
      params_.push_back((*it).asString());
}

bool
CliActionMode::handle(Cli *cli, ParamsMap& input)
{
  if (!name_.empty())
    {
      cli->mode_set(name_);

      // Derive parameters.
      cli->params_.clear();

      for (StringVector::iterator it = params_.begin();
           it != params_.end(); ++it)
        {
          ParamsMap::iterator is = input.find(*it);

          if (is != input.end())
            cli->params_[*it] = is->second;
        }
    }
  else if (up_)
    cli->mode_up(up_);

  return true;
}


bool
CliActionBuiltIn::handle(Cli *cli, ParamsMap& input)
{
  (void)input;

  //TODO
  StringVector vec;

  if (cli->built_in_[func_])
    cli->built_in_[func_](cli, vec);

  return true;
}
