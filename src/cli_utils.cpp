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

#include "project.hpp"
#include "cli_utils.hpp"

// For given Area ID (either IP address or numeric format), return Area ID in
// numeric format and type of format.
//
// Input:
//   0: Area ID in IP address format
//   1: Area ID as numeric format
//
// Output:
//   0: Area ID as unsigned integer
//   1: Area ID format (0: IP address format, 1: numeric format)
//
StringVector&
area_id_and_format(StringVector& vec)
{
  // Save all parameters in local vairables.
  string addr(vec[0]);
  string num(vec[1]);
  const char *format_str;
  char area_id_str[11];

  vec.clear();

  if (!addr.empty())
    {
      struct in_addr a;
      unsigned int ret;
  
      ret = inet_aton(addr.c_str(), &a);
      // This is not expected, since the input has to be validated ealier
      // by parser, so this is just in case.
      if (ret == INADDR_NONE)
        area_id_str[0] = '\0';
      else
        snprintf(area_id_str, 11, "%u", ntohl(a.s_addr));

      format_str = "0";
    }
  else
    {
      snprintf(area_id_str, 11, "%s", addr.c_str());
      format_str = "1";
    }

  vec.push_back(area_id_str);
  vec.push_back(format_str);

  return vec;
}

// For given prefix (A.B.C.D/M format), return network address and masklen.
//
// Input:
//   0: IPv4 prefix (A.B.C.D/M)
//
// Output:
//   0: IPv4 network address (A.B.C.D)
//   1: Mask length (<0-32>)
//
StringVector&
ipv4prefix2address_masklen(StringVector& vec)
{
  string prefix(vec[0]);
  string address;
  string masklen;
  unsigned int pos;

  vec.clear();

  pos = prefix.find("/");
  address = prefix.substr(0, pos);
  masklen = prefix.substr(pos + 1, string::npos);

  vec.push_back(address);
  vec.push_back(masklen);

  return vec;
}


void
CliUtils::init()
{
  filter_map_["area_id_and_format"] = area_id_and_format;
  filter_map_["ipv4prefix2address_masklen"] = ipv4prefix2address_masklen;
}

bool
CliUtils::bind_if_get_token(string& str, string& token)
{
  const char *p = str.c_str();
  size_t pos;

  // skip spaces.
  pos = strspn(p, " ");
  if (pos > 0)
    str = str.substr(pos, string::npos);

  // string is empty.
  if (str.empty())
    return false;

  p = str.c_str();

  if (*p == '(')
    {
      token = "(";
      pos = 1;
    }
  else if (*p == ')')
    {
      token = ")";
      pos = 1;
    }
  else if (*p == ',')
    {
      token = ",";
      pos = 1;
    }
  else
    {
      pos = strcspn(p, " (),");
      token = str.substr(0, pos);
    }

  str = str.substr(pos, string::npos);

  return true;
}

bool
CliUtils::bind_if_interpreter(string& statement, TokenInputMap& input)
{
  string str(statement);
  string lvalues;
  string rvalues;
  string token;
  string tmp_token;
  string func_name;
  StringVector values;
  unsigned int pos;
  bool in_params = false;
  StringVector params;

  // First separate left values and right values.
  pos = str.find("=");
  if (pos == string::npos)
    return false;

  lvalues = str.substr(0, pos);
  rvalues = str.substr(pos + 1, string::npos);

  // Proces rvalues to get list of converted strings.
  while (bind_if_get_token(rvalues, token))
    {
      if (token == "(")
        {
          in_params = true;
          func_name = tmp_token;
          tmp_token.clear();
        }
      else if (token == ")")
        {
          in_params = false;

          // Call filter function.
          CliFilterFuncMap::iterator is = filter_map_.find(func_name);
          if (is != filter_map_.end())
            {
              StringVector vec = is->second(params);
              for (StringVector::iterator it = vec.begin();
                   it != vec.end(); ++it)
                values.push_back(*it);
            }

          params.clear();
          tmp_token.clear();
        }
      else if (token == ",")
        {
          if (!in_params)
            if (!tmp_token.empty())
              values.push_back(input[tmp_token]);
        }
      else // Regular token.
        {
          if (in_params)
            params.push_back(input[token]);
          // Save token.
          else
            tmp_token = token;              
        }
    }

  int i = 0;

  // Bind rvalues to lvalues.
  while (bind_if_get_token(lvalues, token))
    {
      if (token == ",")
        continue;

      if (values.size() <= i)
        break;

      input[token] = values[i];
      i++;
    }

  return true;
}