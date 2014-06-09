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
require 'erb'

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

# Utilities
def keyword(k)
  k.gsub(/\-/, "_")
end

def keyword_dashed(k)
  k.gsub(/_/, "-")
end

def keyword_camel(k)
  k.split(/[\-_]/).map(&:capitalize).join("")
end

def keyword_plural(k)
  keyword(k) + "s"
end

# Rails related
def rails_data_type(obj)
  type = obj["type"]
  if type == "enum"
    type = "string"
  elsif type == "ipv4"
    type = "binary{4}"
  elsif type == "ipv6"
    type = "binary{16}"
  end

  type
end

def rails_db_add_index(table_name, table_def, table_keys)
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
    
    # All index keys
    keys = table_keys.keys
    keys_str = keys.map {|k| '"' + keyword(k) + '"'}.join(", ")
    index_name = name + "_index"
    File.open(migration_file, "w") do |f|
      f.puts lines[0]
      f.puts lines[1]
      f.puts '    add_index "' + name +
        '", [' + keys_str + "], :unique => true, :name => '#{index_name}'"
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

  if attributes != nil
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
  end

  default_value
end

def rails_db_set_default(table_name, table_def)
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
          key = keyword_dashed(m[1])
          default_value = rails_attr_get_default(table_def["attributes"], key)
          line.chomp!
          if default_value != nil
            line += ", :null => false, :default => " + default_value.to_s
          else
            line += ", :null => true"
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

def find_by_keys_statement_str(keys)
  "find_by_" +
    keys.map {|k| keyword(k)}.join("_and_") + "(" +
    keys.map {|k| "params[:" + keyword(k) + "]"}.join(", ") + ")"
end

def rails_modify_model(table_name, table_def, table_keys)
  @model_name = keyword(table_name)
  model = "app/models/" + @model_name + ".rb"
  template = File.read("../model.erb")

  File.open(model, "w") do |f|
    @class_name = keyword_camel(table_name)
    @parent_name = nil
    @parent_class_name = nil
    if table_def["parent"] != nil
      @parent_name = keyword(table_def["parent"]);
      @parent_class_name = keyword_camel(@parent_name)
    end
    @children = table_def["children"]
    @keys_def = table_def["keys"]
    @attrs_def = table_def["attributes"]
    @all_keys = table_keys.keys

    @default_def = Hash.new
    if @attrs_def != nil
      @attrs_def.each do |k, v|
        if v["default"] != nil
          @default_def[keyword(k)] = rails_attr_get_default(@attrs_def, k)
        end
      end
    end

    renderer = ERB.new(template, nil, '<>')
    f.puts renderer.result()
  end
end

def rails_modify_helper(table_name, table_def)
  @helper_name = keyword_plural(table_name)
  helper = "app/helpers/" + @helper_name + "_helper.rb"
  template = File.read("../helper.erb")

  File.open(helper, "w") do |f|
    @class_name = keyword_camel(@helper_name)
    @attrs_def = table_def["attributes"]

    @default_def = Hash.new
    if @attrs_def != nil
      @attrs_def.each do |k, v|
        if v["default"] != nil
          @default_def[keyword(k)] = rails_attr_get_default(@attrs_def, k)
        end
      end
    end

    renderer = ERB.new(template, nil, '<>')
    f.puts renderer.result()
  end
end

def rails_api_path(table_name, table_keys)
  path = Array.new
  path << "api"
  path << keyword_plural(table_name)
  path << table_keys.keys.map {|k| ":" + keyword(k)}
  path.join("/")
end

def rails_modify_controller(table_name, table_def, table_keys)
  controller_name = keyword_plural(table_name)
  controller = "app/controllers/#{controller_name}_controller.rb"
  template = File.read("../_controller.erb")

  File.open(controller, "w") do |f|
    @controller_name = keyword_plural(table_name)
    @model_name = keyword(table_name)
    @class_name = keyword_camel(table_name)
    @api_path = rails_api_path(table_name, table_keys)

    fields = Hash.new
    if table_keys != nil
      fields.merge!(table_keys)
    end
    if table_def["attributes"] != nil
      fields.merge!(table_def["attributes"])
    end

    keys = Array.new
    if table_def["parent"] != nil
      keys << keyword(table_def["parent"]) + "_id"
    end
    keys += fields.keys

    @keys_str = keys.map {|k| ':' + keyword(k)}.join(", ")
    @ipaddr_keys = fields.select {|k, v| v["type"] == "ipv4" || v["type"] == "ipv6"}

    renderer = ERB.new(template, nil, '<>')
    f.puts renderer.result()
  end
end  

def key_value_pairs(name, obj)
  obj.map {|k, v| keyword(k) + " <%= " + name + "." + keyword(k) + " %>"}.join(" ")
end  

# It is a little bit cumbersome to genearete ERB from ERB...
# So we do this way.
def rails_generate_view(table_name, table_def)
  name = keyword(table_name)
  namep = keyword_plural(table_name)
  view = "app/views/" + namep + "/index.cli.erb"

  keys = table_def["keys"]
  attrs = table_def["attributes"]

  # Generate only if it doesn't exist
  if !File.exists?(view)
    File.open(view, "w") do |f|
      f.puts "! " + name + " config"
      f.puts "<% @#{namep}.each do |#{name}| %>"
      f.puts "!"

      # Placeholder of config container, keys must be present.
      if keys != nil
        f.puts name + " " + key_value_pairs(name, keys)
      end

      # Iterate each columns
      if attrs != nil
        attrs.each do |k, v|
          key = keyword(k)

          f.puts "<% if #{name}.#{key} != get_default(:#{key}) %>"
          f.puts " #{k} <%= #{name}.#{key} %>"
          f.puts "<% end %>"
        end
      end

      f.puts "!"
      f.puts "<% end %>"
    end
  end
end

def rails_keys_constraints(table_keys)
  cons = ""

  if table_keys != nil
    keys = table_keys.select {|k, v| v["type"] == "ipv4"}
    if keys.size > 0
      cons = ", constraints: {" +
        keys.map {|k, v| keyword(k) + ': /[\d\.]+/'}.join(', ') + "}"
    end
  end
  
  cons
end

def rails_add_routes(table_name, table_keys)
  name = keyword_plural(table_name)
  routes = "config/routes.rb"
  
  lines = Array.new
  File.open(routes, "r") do |f|
    while line = f.gets
      lines << line
    end
  end

  lines.pop
  File.open(routes, "w") do |f|
    path = rails_api_path(table_name, table_keys)
    cons = rails_keys_constraints(table_keys)

    f.puts lines
    f.puts
    f.puts '  put "' + path + '", to: "' + name + '#create_by_keys"' + cons
    f.puts '  post "' + path + '", to: "' + name + '#update_by_keys"' + cons
    f.puts '  delete "' + path + '", to: "' + name + '#destroy_by_keys"' + cons
    f.puts "end"
  end
end

def rails_scaffolding(dir, name, parent_keys_def)
  table_keys = Hash.new
  table_keys.merge!(parent_keys_def) if parent_keys_def != nil

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

      # Push keys
      if table_def["keys"] != nil
        table_keys.merge!(table_def["keys"])
      end

      # Table keys
      table_keys.each do |k, obj|
        key = keyword(k)
        fields << key + ":" + rails_data_type(obj)
      end

      # Other columns
      if table_def["attributes"] != nil
        table_def["attributes"].each do |k, obj|
          key = keyword(k)
          fields << key + ":" + rails_data_type(obj)
        end
      end

      # We do scaffolding only if model doesn't exist.
      # We should have better granularity.
      model = "app/models/" + keyword(table_name) + ".rb"
      if !File.exists?(model)
        # Scaffolding
        rails_cmd = "rails generate scaffold " +
          keyword(table_name) + " " + fields.join(" ")
        puts rails_cmd
        system(rails_cmd)

        # Migration: generate index's
        rails_db_add_index(table_name, table_def, table_keys)

        # Migration: set default value
        rails_db_set_default(table_name, table_def)

        # Model: add association
        rails_modify_model(table_name, table_def, table_keys)

        # Controller: add custom update/destroy methods
        rails_modify_controller(table_name, table_def, table_keys)

        # Helper: add get_default
        rails_modify_helper(table_name, table_def)

        # View: generate cli.erb
        rails_generate_view(table_name, table_def)

        # Routes: add routes
        rails_add_routes(table_name, table_keys)

        # Iterate children recursively
        if children != nil
          children.each do |c|
            rails_scaffolding(dir, keyword(c), table_def["keys"])
          end
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

def rails_update_mime_types
  mime_types = "config/initializers/mime_types.rb"
  line = 'Mime::Type.register_alias "text/plain", :cli'

  str = File.read(mime_types)
  if !/#{line}/.match(str)
    File.open(mime_types, "w") do |f|
      f.write str
      f.puts
      f.puts line
    end
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

  # Create rails project
  if !File.exists?(rails_project)
    system("rails new #{rails_project}")
  end
  Dir.chdir(rails_project)

  # Read tables.json to get list of parents.
  parents = rails_get_parents(table_json_dir + "/tables.json")

  # Iterate from parents, and then iterate children recursively.
  parents.each do |p|
    rails_scaffolding(table_json_dir, keyword(p), nil)
  end

  # Update mime.types
  rails_update_mime_types

  # rake db:migrate
  system("rake db:migrate")
end

# Start from here
main(rails_project, dir)
