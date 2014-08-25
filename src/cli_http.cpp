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

#include "cli.hpp"
#include "cli_http.hpp"

void
CliHttp::send_request()
{
  try
    {
      Cleanup myCleanup;
      Easy req;

      req.setOpt(new options::Url(url_));
      if (is_debug_)
        req.setOpt(new options::Verbose(true));

      list<string> header;
      header.push_back("Content-type: application/json");

      req.setOpt(new options::HttpHeader(header));
      req.setOpt(new options::CustomRequest(method_));
      req.setOpt(new options::WriteStream(&result_));

      if (method_ != "GET")
        {
          req.setOpt(new options::PostFields(data_));
          req.setOpt(new options::PostFieldSize(data_.size()));
        }

      req.perform();
      status_ = infos::ResponseCode::get(req);
    }
  catch (curlpp::RuntimeError& e)
    {
      if (is_debug_)
        cout << e.what() << std::endl;
    }
  catch (curlpp::LogicError& e)
    {
      if (is_debug_)
        cout << e.what() << std::endl;
    }
}
