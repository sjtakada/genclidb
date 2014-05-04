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

# Setup Rails App and DB environment.
#

require 'pathname'
require 'json'

# Parameters
if ARGV[0] != nil
  rails_project = ARGV[0]
else
  rails_project = "railsapp"
end  

if ARGV[1] != nil
  dir = ARGV[1]
else
  dir = "../db"
end

#
#

def rails_data_type(obj)
  type = obj["type"]
  if type == "enum"
    type = "string"
  elsif type == "ipv4"
    type = "string{11}"
  elsif type == "ipv6"
    type = "string{39}"
  end

  type
end

def rails_table_name(table_name)
  table_name.split("-").map(&:capitalize).join("")
end

def rails_add_index(table_name, keys)
  name = table_name.gsub(/\-/, "_") + "s"

  migration = "add_index_to_" + name

  rails_cmd = "rails generate migration " + migration
  system(rails_cmd)

  Dir.chdir("db/migrate")
  m = Dir.entries(".").select {|f| f =~ /#{migration}\.rb$/}

  if m.size == 1
    migration_file = m[0]

    lines = Array.new
    File.open(migration_file, "r") do |f|
      while line = f.gets
        lines << line
      end
    end

    keys_str = keys.map {|k| '"' + k.gsub(/\-/, "_") + '"'}.join(", ")
    File.open(migration_file, "w") do |f|
      f.puts lines[0]
      f.puts lines[1]
      f.puts '    add_index "' + name +
        '", [' + keys_str + '], :unique => true'
      f.puts lines[2]
      f.puts lines[3]
    end

  else
    puts "Couldn't find migration file: " + migration
  end

  Dir.chdir("../..")
end

def rails_scaffolding(json)
  json["table"].each do |table_name, table_def|
    children = table_def["children"]

    fields = Array.new

    # Association
    if table_def["parent"] != nil
      fields << table_def["parent"].gsub(/\-/, "_") + "_id:integer"
    end

    # Table keys
    table_def["keys"].each do |k, obj|
      key = k.gsub(/\-/, "_");
      fields << key + ":" + rails_data_type(obj)
    end

    # Other fields
    table_def["attributes"].each do |k, obj|
      key = k.gsub(/\-/, "_");
      fields << key + ":" + rails_data_type(obj)
    end

    # Scaffolding
    rails_cmd = "rails generate scaffold " +
      rails_table_name(table_name) + " " +
      fields.join(" ")
    puts rails_cmd
    system(rails_cmd)

    # Generate index
    rails_add_index(table_name, table_def["keys"].keys)
  end
end

# Main
def main(rails_project, dir)
  # Get directory where Table JSON located.
  table_json_dir = File.expand_path(Dir.getwd + "/" + dir)
  if !Dir.exists?(table_json_dir)
    puts "Directory " + dir + " does not exist!"
    abort
  end

  # Move to top directory to start.
  script_file = File.expand_path $0
  top_dir = Pathname.new(script_file).dirname
  Dir.chdir(top_dir)

#  puts "rails_project: " + rails_project
#  puts "table_json_dir: " + table_json_dir
#  puts "script_file: " + script_file
#  puts "top_dir: " + top_dir.to_s

  # Create rails project
  if !File.exists?(rails_project)
    system("rails new #{rails_project}")
  end
  Dir.chdir(rails_project)

  # Iterate table.json files
  Dir.entries(table_json_dir).each do |f|
    /\.table\.json$/.match(f) do
      str = File.read(table_json_dir + "/" + f)
      json = JSON.parse(str)

      rails_scaffolding(json)
    end
  end

  # rake db:migrate
#  system("rake db:migrate");
end

# Start from here
main(rails_project, dir)
