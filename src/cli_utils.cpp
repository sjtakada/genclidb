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
//   1: Area ID format ("ipv4" or "integer")
//
StringVector&
CliFunctorAreaIDandFormat::operator() (StringVector& vec) const
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

      format_str = "ipv4";
    }
  else
    {
      snprintf(area_id_str, 11, "%s", num.c_str());
      format_str = "integer";
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
CliFunctorIPv4Prefix2AddressMasklen::operator() (StringVector& vec) const
{
  string prefix(vec[0]);
  string address;
  string masklen;
  size_t pos;

  vec.clear();

  pos = prefix.find("/");
  address = prefix.substr(0, pos);
  masklen = prefix.substr(pos + 1, string::npos);

  vec.push_back(address);
  vec.push_back(masklen);

  return vec;
}

// For given network adddress and masklen, return prefix (A.B.C.D/M).
//
// Input:
//   0: IPv4 network address (A.B.C.D)
//   1: Mask length (<0-32>)
//
// Output:
//   0: IPv4 prefix (A.B.C.D/M)
//
StringVector&
CliFunctorAddressMasklen2IPv4Prefix::operator() (StringVector& vec) const
{
  string address(vec[0]);
  string masklen(vec[1]);
  string prefix;

  vec.clear();

  prefix = address + "/" + masklen;

  vec.push_back(prefix);

  return vec;
}

// For given network address and mask pair, apply mask and return address.
//
// Input:
//   0: IPv4 Network Address (A.B.C.D)
//   1: IPV4 Mask length(<0-32>)
//
// Output:
//   0: IPv4 Network Address (A.B.C.D)
//
StringVector&
CliFunctorApplyMaskIPv4::operator() (StringVector& vec) const
{
  string addr(vec[0]);
  string masklen(vec[1]);
  unsigned int mask;
  struct in_addr a;
  struct in_addr am;

  vec.clear();

  mask = masklen2mask(atoi(masklen.c_str()));

  inet_aton(addr.c_str(), &a);
  am.s_addr = a.s_addr & mask;

  string addr_masked(inet_ntoa(am));
  vec.push_back(addr_masked);

  return vec;
}


void
CliUtils::init()
{
  functor_map_["area_id_and_format"] =
    new CliFunctorAreaIDandFormat;
  functor_map_["ipv4prefix2address_masklen"] =
    new CliFunctorIPv4Prefix2AddressMasklen;
  functor_map_["address_masklen2ipv4prefix"] =
    new CliFunctorAddressMasklen2IPv4Prefix;
  functor_map_["apply_mask_ipv4"] =
    new CliFunctorApplyMaskIPv4;
}

bool
CliUtils::get_bind_token(string& str, string& token)
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

void
CliUtils::expand(string& statement, ParamsMap& input, StringVector& values)
{
  string token;
  string func_name;
  bool in_params = false;
  string tmp_token;
  StringVector params;

  // Process rvalues to get list of converted strings.
  while (get_bind_token(statement, token))
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
          CliFunctorMap::iterator is = functor_map_.find(func_name);
          if (is != functor_map_.end())
            {
              StringVector vec = (*is->second)(params);
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
              {
                if (input.find(tmp_token) != input.end())
                  values.push_back(input[tmp_token]);
                else
                  values.push_back(tmp_token);
              }
        }
      else // Regular token.
        {
          if (in_params)
            {
              if (token == "NULL")
                params.push_back("");
              else
                params.push_back(input[token]);
            }
          // Save token.
          else
            tmp_token = token;              
        }
    }

  if (!tmp_token.empty())
    {
      if (input.find(tmp_token) != input.end())
        values.push_back(input[tmp_token]);
      else
        values.push_back(tmp_token);
    }
}

bool
CliUtils::bind_interpreter(string& statement, ParamsMap& input)
{
  string str(statement);
  string lvalue, rvalue;
  string token;
  StringVector values;
  size_t pos;

  // First separate left values and right values.
  pos = str.find("=");
  if (pos == string::npos)
    return false;

  lvalue = str.substr(0, pos);
  rvalue = str.substr(pos + 1, string::npos);

  // Process statement
  expand(rvalue, input, values);

  unsigned int i = 0;

  // Bind rvalues to lvalues.
  while (get_bind_token(lvalue, token))
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

void
CliUtils::json_to_params(Json::Value& obj, ParamsMap& params)
{
  for (Json::Value::iterator it = obj.begin(); it != obj.end(); ++it)
    params[it.key().asString()] = (*it).asString();
}

void
CliUtils::candidates_by_field(Json::Value& json_resp,
                              string& field, StringVector& candidates)
{
  for (Json::Value::iterator it = json_resp.begin();
       it != json_resp.end(); ++it)
    {
      Json::Value record = (*it);
      ParamsMap params;
      string str(field);
  
      json_to_params(record, params);

      expand(str, params, candidates);
    }
}

