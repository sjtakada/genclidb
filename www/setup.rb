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

def keyword(k)
  k.gsub(/\-/, "_")
end

def rkeyword(k)
  k.gsub(/_/, "-")
end

def keyword_plural(k)
  keyword(k) + "s"
end

def rails_add_index(table_name, keys)
  name = keyword_plural(table_name)
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
        next if line =~ /add_index/

        lines << line
      end
    end

    keys_str = keys.map {|k| '"' + keyword(k) + '"'}.join(", ")
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

def rails_attr_get_default(attributes, key)
  default_value = nil

  if attributes[key] != nil
    attr = attributes[key]
    if attr["default"] != nil
      if attr["type"] == "boolean" or attr["type"] == "integer"
        return attr["default"].to_s
      else
        return '"' + attr["default"] + '"'
      end
    end
  end

  default_value
end

def rails_set_default(table_name, table_def)
  name = keyword_plural(table_name)
  migration = "create_" + name

  Dir.chdir("db/migrate")
  m = Dir.entries(".").select {|f| f =~ /#{migration}\.rb$/}

  if m.size == 1
    migration_file = m[0]

    lines = Array.new
    File.open(migration_file, "r") do |f|
      while line = f.gets
        m = /^\s+t\.\w+ :(\w+)/.match(line)
        if m != nil
          key = rkeyword(m[1])
          default_value = rails_attr_get_default(table_def["attributes"], key)
          if default_value != nil
            line.chomp!
            line += ", :null => false, :default => " + default_value.to_s
          end
        end

        lines << line
      end
    end

    File.open(migration_file, "w") do |f|
      lines.each do |line|
        f.puts line
      end
    end
  end

  Dir.chdir("../..")
end

def rails_modify_model(table_name, table_def)
  name = keyword(table_name)

  model = "app/models/" + name + ".rb"
  if File.exists?(model)
    lines = Array.new
    File.open(model, "r") do |f|
      while line = f.gets
        next if line =~ /belongs_to/
        next if line =~ /has_many/

        lines << line
      end
    end

    # last line must be "end"
    lines.pop

    File.open(model, "w") do |f|
      f.puts lines[0]

      if table_def["parent"] != nil
        f.puts "  belongs_to :" + keyword_plural(table_def["parent"])
      end

      if table_def["children"] != nil
        table_def["children"].each do |k, v|
          f.puts "  has_many :" + keyword(k) + ", dependent: :destroy"
        end
      end

      # integer range validation
      table_def["keys"].each do |k, v|
        if v["type"] == "integer" and v["range"] != nil
          f.puts "  validates :" + keyword(k) + ", :numericality => {" +
            ":greater_than_or_equal_to => " + v["range"][0].to_s + ", " +
            ":less_than_or_equal_to => " +v["range"][1].to_s + "}"
        end
      end

      table_def["attributes"].each do |k, v|
        if v["type"] == "integer" and v["range"] != nil
          f.puts "  validates :" + keyword(k) + ", :numericality => {" +
            ":greater_than_or_equal_to => " + v["range"][0].to_s + ", " +
            ":less_than_or_equal_to => " +v["range"][1].to_s + "}"
        end
      end

      f.puts "end"
    end
  end
end

def rails_scaffolding(dir, name, table_keys)
  f = dir + "/" + name + ".table.json"
  if File.exists?(f)
    str = File.read(f)
    json = JSON.parse(str)

    json["table"].each do |table_name, table_def|
      children = table_def["children"]

      fields = Array.new

      # Association
      if table_def["parent"] != nil
        fields << keyword(table_def["parent"]) + "_id:integer"
      end

      # Table keys
      table_def["keys"].each do |k, obj|
        key = keyword(k)
        fields << key + ":" + rails_data_type(obj)
      end

      # Save keys
      table_keys[name] = table_def["keys"]

      # Other columns
      table_def["attributes"].each do |k, obj|
        key = keyword(k)
        fields << key + ":" + rails_data_type(obj)
      end

      # Scaffolding
      rails_cmd = "rails generate scaffold " +
        keyword(table_name) + " " + fields.join(" ")
      puts rails_cmd
      system(rails_cmd)

      # Generate indexes
      rails_add_index(table_name, table_def["keys"].keys)

      # Set default value
      rails_set_default(table_name, table_def)

      # Add Association to models
      rails_modify_model(table_name, table_def)

      # Iterate children recursively
      if children != nil
        children.each do |c|
          rails_scaffolding(dir, keyword(c), table_keys)
        end
      end
    end
  end
end

def rails_get_parents(tables_json)
  parents = Array.new

  if File.exists?(tables_json)
    str = File.read(tables_json)
    json = JSON.parse(str)

    if json["tables"] != nil
      json["tables"].each do |p|
        parents << p
      end
    end
  end

  parents
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

  # Create rails project
  if !File.exists?(rails_project)
    system("rails new #{rails_project}")
  end
  Dir.chdir(rails_project)

  # Read tables.json to get list of parents.
  parents = rails_get_parents(table_json_dir + "/tables.json")

  # Prepare parent keys hash, in order to complete controller.
  table_keys = Hash.new

  # Iterate from parents, and then iterate children recursively.
  parents.each do |p|
    rails_scaffolding(table_json_dir, keyword(p), table_keys)
  end

#  table_keys.each do |k, v|
#    puts "name:" + k
#    puts "keys:" + v.keys.to_s
#  end

  # rake db:migrate
  system("rake db:migrate")
end

# Start from here
main(rails_project, dir)
