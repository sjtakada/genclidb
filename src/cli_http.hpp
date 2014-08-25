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
#ifndef _CLI_HTTP_HPP_
#define _CLI_HTTP_HPP_

#include <iostream>

#include "project.hpp"

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Exception.hpp>

#include "json/json.h"

using namespace curlpp;

class CliHttp
{
public:
  CliHttp(string& url, string& method, string& data, bool is_debug)
    : url_(url), method_(method), data_(data),
      is_debug_(is_debug), runtime_error_(false)
  { }

  static bool get_candidate(Cli *cli, string& path, StringVector& candidates);

  bool runtime_error() { return runtime_error_; }
  stringstream& result() { return result_; }
  int status() { return status_; }
  void send_request();

private:
  // Request URL.
  string url_;

  // HTTP method.
  string method_;

  // HTTP request data.
  string data_;

  // To show debug information.
  bool is_debug_;

  // Runtime error.
  bool runtime_error_;

  // HTTP response.
  stringstream result_;

  // HTTP status code.
  int status_;
};

#endif /* _CLI_HTTP_HPP_ */
