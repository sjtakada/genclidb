#!/usr/bin/ruby
# -*- coding: utf-8 -*-
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
require 'getoptlong'
require 'json'
require 'erb'

# Mandatory arguments
#  [0] rails project
#  [1] table.json directory
#
if ARGV[0] != nil
  $rails_project = ARGV[0]
else
  $rails_project = "railsapp"
end  

if ARGV[1] != nil
  db_dir = ARGV[1]
else
  db_dir = "../db"
end

def show_help
  puts "Usage: this-script <rails-project> <table-def-dir> [options...]"
  exit
end

def set_options_all(options)
  options[:scaffold] = true
  options[:migration] = true
  options[:model] = true
  options[:controller] = true
  options[:view] = true
  options[:helper] = true
  options[:routes] = true
  options[:mime] = true
end

def unset_options_all(options)
  options[:scaffold] = false
  options[:migration] = false
  options[:model] = false
  options[:controller] = false
  options[:view] = false
  options[:helper] = false
  options[:routes] = false
  options[:mime] = false
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

def rails_db_add_index(table_name, table_keys)
  puts "=> Add index to '" + table_name + "' migrations..."

  index_keys = table_keys.map{|t| t[0]}
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
    keys_str = index_keys.map {|k| '"' + keyword(k) + '"'}.join(", ")
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
  default_value = "nil"

  if attributes != nil
    if attributes[key] != nil
      attr = attributes[key]
      if attr.has_key?("default")
        if attr["type"] == "boolean" or attr["type"] == "integer"
          return attr["default"].to_s
        else
          return '"' + attr["default"] + '"'
        end
      else
        default_value = "nil"
      end
    end
  end

  default_value
end

def rails_db_set_default(table_name)
  puts "=> Add default values to '" + table_name + "' migrations..."

  table_def = $table2json[table_name]
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
          if default_value != "nil"
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

def find_by_assoc_keys_statement_str(keys)
  "find_by_" +
    keys.map {|k| keyword(k) + "_id"}.join("_and_") + "(" +
    keys.map {|k| keyword(k) + ".id"}.join(", ") + ")"
end

def find_by_func_name_str(table_keys)
  "find_by_" + table_keys.map{|t| keyword(t[0])}.join("_and_")
end

def rails_modify_model(table_name, table_keys)
  puts "=> Modify model '" + table_name + "' ..."

  table_def = $table2json[table_name]
  @is_assoc = (table_def["type"] == "association") ? true : false

  @model_name = keyword(table_name)
  model = "app/models/" + @model_name + ".rb"
  template = File.read("../model.erb")

  File.open(model, "w") do |f|
    @class_name = keyword_camel(table_name)
    @parents = table_def["belongs-to"]
    @polymorphic = table_def["belongs-to-polymorphic"]
    @children = table_def["has-dependent"]
    @keys_def = table_def["keys"]
    @attrs_def = table_def["attributes"]
    @all_keys = table_keys
    @associations = table_def["has-association"]

    @default_def = Hash.new
    if @attrs_def != nil
      @attrs_def.each do |k, v|
        @default_def[keyword(k)] = rails_attr_get_default(@attrs_def, k)
      end
    end

    @ipaddr_keys = table_keys.select {|t|
      t[0]["type"] == "ipv4" || t[0]["type"] == "ipv6"
    }

    renderer = ERB.new(template, nil, '<>')
    f.puts renderer.result()
  end
end

def rails_modify_helper(table_name, table_def)
  puts "=> Modify helper '" + table_name + "'..."

  @helper_name = keyword_plural(table_name)
  helper = "app/helpers/" + @helper_name + "_helper.rb"
  template = File.read("../helper.erb")

  File.open(helper, "w") do |f|
    @class_name = keyword_camel(@helper_name)
    @attrs_def = table_def["attributes"]

    @default_def = Hash.new
    if @attrs_def != nil
      @attrs_def.each do |k, v|
        @default_def[keyword(k)] = rails_attr_get_default(@attrs_def, k)
      end
    end

    renderer = ERB.new(template, nil, '<>')
    f.puts renderer.result()
  end
end

def rails_api_path(table_name, heritage)
  path = Array.new
  path << "api"

  heritage.each do |tuple|
    name, t = tuple[0], tuple[1]

    if t["type"] == "dependent" and !t["alias"].nil?
      path << keyword_plural(t["alias"])
    else
      path << keyword_plural(name)
    end

    if !t["keys"].nil?
      t["keys"].each do |k, v|
        path << ":" + keyword(k)
      end
    end
  end

  path.join("/")
end

def rails_modify_controller(table_name, table_keys, db_fields, api_path)
  puts "=> Modify controller '" + table_name + "'..."

  table_def = $table2json[table_name]
  @is_assoc = (table_def["type"] == "association") ? true : false

  @controller_name = keyword_plural(table_name)
  controller = "app/controllers/#{@controller_name}_controller.rb"
  template = File.read("../controller.erb")

  File.open(controller, "w") do |f|
    @parents = table_def["belongs-to"]
    @model_name = keyword(table_name)
    @class_name = keyword_camel(table_name)
    @api_path = api_path

    @order = nil
    if table_def["order"] != nil
      @order = table_def["order"]
    end

    @keys_str = db_fields.map {|f| ':' + keyword(f[0])}.join(", ")

    renderer = ERB.new(template, nil, '<>')
    f.puts renderer.result()
  end
end  

def key_value_pairs(name, obj)
  obj.map {|k, v| keyword(k) + " <%= " + name + "." + keyword(k) + " %>"}.join(" ")
end  

# It is a little bit cumbersome to genearete ERB from ERB...
# So we do this way.
def rails_generate_view(table_name)
  puts "=> Generate views '" + table_name + "' ..."

  table_def = $table2json[table_name]
  name = keyword(table_name)
  namep = keyword_plural(table_name)
  class_name = keyword_camel(table_name)

  keys = table_def["keys"]
  attrs = table_def["attributes"]

  # Generate only if it doesn't exist
  _view = "app/views/" + namep + "/_index.cli.erb"
  if !File.exists?(_view)
    File.open(_view, "w") do |f|
      f.puts "<% #{namep}.each do |v| %>"

      # Placeholder of config container, keys must be present.
      if keys != nil
        f.puts name + " " + key_value_pairs('v', keys)
      end

      # Iterate each columns
      if attrs != nil
        attrs.each do |k, v|
          key = keyword(k)

          f.puts "<%   if v.#{key} != #{class_name}.get_default(:#{key}) %>"
          f.puts " #{k} <%= v.#{key} %>"
          f.puts "<%   end %>"
        end
      end

      f.puts "<% end %>"
    end
  end

  # Generate only if it doesn't exist
  view = "app/views/" + namep + "/index.cli.erb"
  if !File.exists?(view)
    File.open(view, "w") do |f|
      f.puts "!"
      f.puts "! " + name + " config"
      f.puts "!"
      f.puts "<%= render partial: \"#{namep}/index\", locals: {#{namep}: @#{namep}} -%>"

      f.puts "!"
    end
  end
end

def rails_keys_constraints(table_keys)
  cons = ""

  if table_keys != nil
    keys = table_keys.select {|t| t[1]["type"] == "ipv4" || t[1]["type"] == "ipv6"}
    if keys.size > 0
      cons = ", constraints: {" +
        keys.map {|t| keyword(t[0]) + ': /[\d\.]+/'}.join(', ') + "}"
    end
  end
  
  cons
end

def rails_add_routes(table_name, table_keys, api_path)
  puts "=> Add routes '" + table_name + "' ..."

  name = keyword_plural(table_name)

  $routes[name] = Hash.new
  $routes[name][:path] = api_path
  $routes[name][:cons] = rails_keys_constraints(table_keys)
end

def rails_get_heritage(table_name)
  heritage = Array.new

  table_def = $table2json[table_name]
  if !table_def.nil?
    if !table_def["belongs-to"].nil?
      if table_def["belongs-to"].size > 1
        puts ">>> Error: Most likely " + table_name + " is association"
        abort
      end

      heritage += rails_get_heritage(table_def["belongs-to"][0])
    end

    heritage << [table_name, table_def]
  end
  
  heritage
end

def rails_get_table_keys(heritage)
  table_keys = Array.new

  heritage.each do |tuple|
    name, t = tuple[0], tuple[1]
    if !t["keys"].nil?
      t["keys"].each do |k, v|
        table_keys << [k, v]
      end
    end
  end

  table_keys
end

def rails_get_db_fields(table_def, table_keys)
  db_fields = Array.new

  # Association
  if table_def["belongs-to"] != nil
    table_def["belongs-to"].each do |k|
      db_fields << [ keyword(k) + "_id", "integer" ]
    end
  end

  # Polymorphic Association
  if table_def["belongs-to-polymorphic"] != nil
    table_def["belongs-to-polymorphic"].each do |k|
      db_fields << [ keyword(k) + "_id", "integer" ]
      db_fields << [ keyword(k) + "_type", "string" ]
    end
  end

  # Table keys
  table_keys.each do |tuple|
    key = keyword(tuple[0])
    db_fields << [ key, rails_data_type(tuple[1]) ]
  end

  # Other columns
  if table_def["attributes"] != nil
    table_def["attributes"].each do |k, obj|
      key = keyword(k)
      db_fields << [ key, rails_data_type(obj) ]
    end
  end

  db_fields
end

def rails_add_table(table_name)
  table_def = $table2json[table_name]
  if table_def.nil?
    puts ">>> Error: No table defintion for " + table_name
  else
    # Get list of keys from heritage.
    heritage = rails_get_heritage(table_name)
    table_keys = rails_get_table_keys(heritage)
    db_fields = rails_get_db_fields(table_def, table_keys)
    api_path = rails_api_path(table_name, heritage)

    # We do scaffolding only if model doesn't exist.
    # We should have better granularity.
    model = "app/models/" + keyword(table_name) + ".rb"
    if !File.exists?(model) and $options[:scaffold]
      # Scaffolding
      rails_cmd = "rails generate scaffold " +
        keyword(table_name) + " " +
        db_fields.map{|a| a[0] + ":" + a[1] }.join(" ")
      puts "=> Scaffolding " + table_name + " ..."
      puts rails_cmd
      system(rails_cmd)
    end

    if $options[:migration]
      # Migration: generate index's
      rails_db_add_index(table_name, table_keys)

      # Migration: set default value
      rails_db_set_default(table_name)
    end

    # Model: add association
    if $options[:model]
      rails_modify_model(table_name, table_keys)
    end

    # Controller: add custom update/destroy methods
    if $options[:controller]
      rails_modify_controller(table_name, table_keys, db_fields, api_path)
    end

    # View: generate cli.erb
    if $options[:view]
      rails_generate_view(table_name)
    end

    # Routes: add routes
    if $options[:routes]
      rails_add_routes(table_name, table_keys, api_path)
    end

    # Iterate children recursively
    children = table_def["has-dependent"]
    if children != nil
      children.each do |child|
        rails_add_table(child)
      end
    end
  end
end

def rails_add_association(table_name)
  table_def = $table2json[table]
  if table_def.nil?
    puts ">>> Error: No table defintion for " + table_name
  else
    table_keys = Hash.new
    index_keys = Hash.new

    db_fields = Array.new

    # Association, these are keys, too.
    if table_def["belongs-to"] != nil
      table_def["belongs-to"].each do |k|
        key_id = keyword(k) + "_id"

        db_fields << key_id + ":integer"
        table_keys[key_id] = Hash.new
        table_keys[key_id]["type"] = "integer"
        if $table2json[k] != nil
          index_keys.merge!($table2json[k]["keys"])
        end
      end
    end

    # Polymorphic Association
    if table_def["belongs-to-polymorphic"] != nil
      table_def["belongs-to-polymorphic"].each do |k|
        key_id = keyword(k) + "_id"
        key_type = keyword(k) + "_type"

        db_fields << key_id + ":integer"
        db_fields << key_type + ":string"
        table_keys[key_id] = Hash.new
        table_keys[key_id]["type"] = "integer"
        table_keys[key_type] = Hash.new
        table_keys[key_type]["type"] = "string"
#        if $table2json[k] != nil
#          index_keys.merge!($table2json[k]["keys"])
#        end
      end
    end

    # Other columns
    if table_def["attributes"] != nil
      table_def["attributes"].each do |k, obj|
        key = keyword(k)
        db_fields << key + ":" + rails_data_type(obj)
      end
    end

    # We do scaffolding only if model doesn't exist.
    # We should have better granularity.
    model = "app/models/" + keyword(table_name) + ".rb"
    if !File.exists?(model) and $options[:scaffold]
      # Scaffolding
      rails_cmd = "rails generate scaffold " +
        keyword(table_name) + " " + db_fields.join(" ")
      puts "=> Scaffolding " + table_name + " ..."
      puts rails_cmd
      system(rails_cmd)
    end

    if $options[:migration]
      # Migration: generate index's
      rails_db_add_index(table_name, table_keys)

      # Migration: set default value
      rails_db_set_default(table_name)
    end

    # Model: add association
    if $options[:model]
      rails_modify_model(table_name, table_keys)
    end

    # Controller: add custom update/destroy methods
    if $options[:controller]
      rails_modify_controller(table_name, index_keys, db_fields, api_path)
    end

    # View: generate cli.erb
    if $options[:view]
      rails_generate_view(table_name)
    end

    # Routes: add routes
    if $options[:routes]
      rails_add_routes(table_name, index_keys, api_path)
    end
  end
end

def rails_load_tables(dir)
  parents = Array.new
  associations = Array.new
  table2json = Hash.new

  m = Dir.entries(dir).select {|f| f =~ /\.table\.json$/}
  m.each do |f|
    file = dir + "/" + f

    str = File.read(file)
    json = JSON.parse(str)

    if json["table"] != nil
      json["table"].each do |t, obj|
        if obj["type"] != nil
          if obj["type"] == "parent"
            parents << t
          elsif obj["type"] == "association"
            associations << t
          end
        end

        table2json[t] = obj
      end
    end
  end

  puts "parents:"
  parents.each do |p|
    puts "- " + p
  end

  puts ""
  puts "associations:"
  associations.each do |a|
    puts "- " + a
  end

  puts

  [parents, associations, table2json]
end

def rails_update_mime_types
  puts "=> Update mime_types.rb..."

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

def rails_update_routes
  puts "=> Update routes.rb ..."

  routes = "config/routes.rb"
  template = File.read("../routes.erb")

  File.open(routes, "w") do |f|
    @rails_project = $rails_project
    @routes = $routes

    renderer = ERB.new(template, nil, '<>')
    f.puts renderer.result()
  end
end

# Main
def main(dir)
  # Get directory where Table JSON located.
  table_json_dir = File.expand_path(Dir.getwd + "/" + dir)
  if !Dir.exists?(table_json_dir)
    puts ">>> Directory " + dir + " does not exist!"
    abort
  end

  # Move to top directory to start.
  script_file = File.expand_path $0
  top_dir = Pathname.new(script_file).dirname
  Dir.chdir(top_dir)

  # Create rails project
  if !File.exists?($rails_project)
    system("rails new #{$rails_project}")
  end
  Dir.chdir($rails_project)

  # Load *.table.json to get list of parents, association and table2json.
  parents, associations, table2json = rails_load_tables(table_json_dir)
  $table2json = table2json

  # Routes hash.
  $routes = Hash.new

  # Iterate from parents, and then iterate children recursively.
  parents.each do |p|
    rails_add_table(p)
  end

  # Iterate from associations.
  associations.each do |a|
#    rails_add_association(a)
  end

  # Generate routes
  rails_update_routes()

  # Update mime.types
  rails_update_mime_types if $options[:mime] == true

  # rake db:migrate
  puts "=> Run DB migration ..."
  system("rake db:migrate")
end

#
# Options
#
$options = Hash.new
set_options_all($options)

opts = GetoptLong.new(
  [ '--help',            '-h', GetoptLong::NO_ARGUMENT ],
  [ '--model-only',      '-m', GetoptLong::NO_ARGUMENT ],
  [ '--controller-only', '-c', GetoptLong::NO_ARGUMENT ],
  [ '--view-only',       '-v', GetoptLong::NO_ARGUMENT ]
)

opts.each do |opt, arg|
  case opt
    when '--help'
      show_help
    when '--model-only'
      unset_options_all($options)
      $options[:model] = true
      $options[:scaffold] = false
    when '--controller-only'
      unset_options_all($options)
      $options[:controller] = true
      options[:scaffold] = true
    when '--view-only'
      unset_options_all($options)
      $options[:view] = true
      $options[:scaffold] = true
  end
end

# Start from here
main(db_dir)
