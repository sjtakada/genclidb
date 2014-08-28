#!/usr/bin/ruby
#
# Copyright (c) 2014 Toshiaki Takada
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Generate JSON CLI file with *.c in quagga.
#
# Usage: this-script *.c
#

require 'pathname'
require 'json'

def cli_parse_defun(defun_all, line, f)
  defun_str = line
  begin
    line = f.gets
    defun_str += line
  end while line !~ /\)\s*$/

  defun_str.gsub!(/\n/, " ")

  matched = /(DEFUN|ALIAS) \(\w+,\s*(\w+),\s*\"(.*)\",\s*(\".*\")\s*\)/.match(defun_str)
  if matched == nil
    puts "Error: couldn't parse defun: " + defun_str
    exit
  end

  id = matched[2]
  cmdstr = matched[3]
  cmdstr.gsub!(/\"\s+\"/, "")
  cmdstr.gsub!(/\"\"/, "")

  helpstr = matched[4]

  helpstr.sub!(/^\s*\"/, "")
  helpstr.sub!(/\"\s*$/, "")
  helpstr.gsub!(/\"\s+\"/, "")

  help_array = helpstr.split(/\\n/)

  defun = Hash.new
  defun[:cmdstr] = cmdstr
  defun[:helpstr] = helpstr.split(/\\n/)
  defun_all[id] = defun
end

def cli_parse_install(install_all, line, f)
  install_str = line

  while line !~ /\);\s*$/
    line = f.gets
    install_str += line
  end

  install_str.gsub!(/\n/, " ")

  matched = /install_element\s*\((\w+),\s*\&(\w+)\);/.match(install_str)
  if matched == nil
    puts "Error: couldn't parse install_element " + install_str
    return
  end
  
  mode = matched[1]
  id = matched[2]

  if install_all[id] == nil
    install_all[id] = Array.new
  end
  install_all[id] << mode
end

def cli_get_token(str)
  type = :undef

  if str =~ /^(\s+)/
    type = :whitespace
  elsif str =~ /^\|/
    type = :pipe
  elsif str =~ /^\(/
    type = :leftparen
  elsif str =~ /^\[/
    type = :leftbracket
  elsif str =~ /^\)/
    type = :rightparen
  elsif str =~ /^\]/
    type = :rightbracket
  elsif str =~ /^\./
    type = :dot
  elsif str =~ /^A\.B\.C\.D\/M/
    type = :ipv4prefix
  elsif str =~ /^A\.B\.C\.D/
    type = :ipv4address
  elsif str =~ /^X:X::X:X\/M/
    type = :ipv6prefix
  elsif str =~ /^X:X::X:X/
    type = :ipv6address
  elsif str =~ /^\<[[:digit:]]+-[[:digit:]]+\>/
    type = :integer
  elsif str =~ /^\<\+\/-metric>/
    type = :metric_offset
  elsif str =~ /^AA:NN/
    type = :community_new
  elsif str =~ /^HH:MM:SS/
    type = :time
  elsif str =~ /^MONTH/
    type = :month
# TODO: special case for ospf6d
  elsif str =~ /^[A-Z][A-Za-z_0-9:\.\-\/]*/
    type = :string
  elsif str =~ /^[A-Z][A-Z_0-9:\.\-]*/
    type = :string
  elsif str =~ /^[a-z0-9\*][a-zA-Z_0-9:\.\-\*]*/
    type = :keyword
  else
    puts "Error: unknown token '#{str}', abort!"
    exit 1
  end

  token = $~.to_s
  remainder = $~.post_match

  [type, token, remainder]
end

def cli_str2token(defun)
  cmdstr = defun[:cmdstr]
  i = 0
  h = Hash.new
  id = Array.new
  # Make 'no' starting from '0' and other starting from '1',
  # so that params' ID becomes consistent.
  if cmdstr =~ /^no /
    id.push(0)
  else
    id.push(1)
  end
  token_all = Array.new

  dot = false
  while cmdstr != ""
    key = nil
    type = nil
    range = nil

    type, token, remainder = cli_get_token(cmdstr)
    id_str = id.join(".")
    cmdstr = remainder

    case type
    when :whitespace
      ;
    when :pipe
      # This is a tricky part. Zebra/Quagga consider (A|) as "A or nothing",
      # but just "A" as a parameter.  In this case, we ignore latter part
      # after "|", so that this particular token becomes "(A)"
      next if remainder =~ /^\)/;
    when :leftparen
      id.push(0)
    when :rightparen
      id.pop
      id[-1] += 1
    when :leftbracket
      id.push(0)
      token = "("
    when :rightbracket
      id.pop
      id[-1] += 1
      token = "|)"
    when :dot
      dot = true;
      token = "[";
    when :ipv4prefix
      key = "IPV4-PREFIX:" + id_str
    when :ipv4address
      key = "IPV4-ADDR:" + id_str
    when :ipv6prefix
      key = "IPV6-PREFIX:" + id_str
    when :ipv6address
      key = "IPV6-ADDR:" + id_str
    when :integer
      key = "RANGE:" + id_str
      matched = /^\<([[:digit:]]+)-([[:digit:]]+)\>/.match(token)
      range = [ matched[1].to_i, matched[2].to_i ]
    when :metric_offset
      key = "METRIC-OFFSET:" + id_str
    when :community_new
      key = "COMMUNITY:" + id_str
    when :string
      key = "WORD:" + id_str
    when :time
      key = "TIME:" + id_str
    when :month
      key = "MONTH:" + id_str
    when :string_array
      key = "ARRAY:" + id_str
    when :keyword
      key = token
    end

    if key != nil
      token_all << key
      if dot == true
        token_all << "]" 
        dot = false
      end
    else
      token_all << token
      next
    end

    h[key] = Hash.new
    h[key]['id'] = id_str
    h[key]['type'] = type
    if defun[:helpstr][i] != nil
      h[key]['help'] = defun[:helpstr][i]
    else
      h[key]['help'] = "*MISSING-HELP*"
    end
    if range != nil
      h[key]['range'] = range
    end

    if type == :keyword and /\./.match(id_str)
      id_enum = id_str.gsub(/\.\d+/, '')
      h[key]['enum'] = 'ENUM:' + id_enum
    end

    id[-1] += 1
    i += 1
  end

  cmdout = token_all.join("")

  [h, cmdout]
end

pwd = Dir.pwd
# Iterate all *.c files
ARGV.each do |filename|
  path = Pathname.new(filename)
  dir = path.dirname.to_s
  basename = path.basename.to_s

  # Goto source directory.
  Dir.chdir(dir);

  defun_all = Hash.new
  install_all = Hash.new

  # Parse *.c to retrieve (DEFUN|ALIAS) and install_element.
  f = IO.popen("gcc -E -DHAVE_CONFIG_H -DVTYSH_EXTRACT_PL -DHAVE_IPV6 -I.. -I./ -I./.. -I../lib -I../lib -I../isisd/topology #{basename}")
  while line = f.gets
    line.chomp!

    if line =~ /^DEFUN / or line =~ /^ALIAS /
      cli_parse_defun(defun_all, line, f)
    elsif line =~ /^\s+install_element[^_]/
      cli_parse_install(install_all, line, f)
    end
  end
  f.close

  # Go back to CLIJSON directory
  Dir.chdir(pwd);

  # Read JSON file if it exists.
  cli_json_file = basename
  cli_json_file.sub!(/\.c$/, ".cli.json")

  cli_json = nil
  if File.exists?(cli_json_file)
    string = File.read(cli_json_file)
    cli_json = JSON.parse(string);
  else
    cli_json = Hash.new
  end

  if defun_all.count > 0
    # Generate CLI JSON.
    defun_all.each do |id, v|
      idx = id.gsub(/_/, "-")

      # Find if command already exists.
      h = nil
      if cli_json[idx] != nil
        h = cli_json[idx]
      else
        h = Hash.new
        cli_json[idx] = h
      end

      # Overwrite token and command defun.
      h['token'], cmdout = cli_str2token(v)

      # Preserve array.
      if h['command'] == nil
        h['command'] = Array.new
        command = Hash.new
        h['command'] << command
      else
        command = h['command'][0]
      end

      command['defun'] = cmdout

      modes = nil
      if install_all[id] != nil
        modes =  install_all[id].map {|s| s.gsub(/_/, "-") }
      else
        modes = Array.new
      end

      if command['mode'] != nil
        modes += command['mode']
      end

      command['mode'] = modes.uniq
      command.delete('mode') if modes.size == 0

# temporary conversion code
      if command['action'] != nil
#        new_action = Array.new
#
#        command['action'].each do |a|
#          a.each do |k, obj|
#            if k != '*'
#              obj['cond'] = k
#            end
#            new_action << obj
#          end
#        end

#        command['action'] = new_action
# temporary conversion code end
      else
        action = Hash.new
        action['http'] = Hash.new
        action['http']['method'] = "NOP"
        action['http']['path'] = ""
        action['http']['params'] = Hash.new

        command['action'] = Array.new
        command['action'] << action
      end
    end

    # Write JSON file with same basename.
    File.open(cli_json_file, "w") do |f|
      f.write(JSON.pretty_generate(cli_json))
    end
  end
end

