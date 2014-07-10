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
require 'getoptlong'
require 'json'
require 'erb'

# Mandatory arguments
#  [0] rails project
#  [1] table.json directory
#
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

def rails_db_add_index(table_name, table_def, table_keys)
  puts "=> Add index to '" + table_name + "' migrations..."

  index_keys = table_keys.keys
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

def rails_db_set_default(table_name, table_def)
  puts "=> Add default values to '" + table_name + "' migrations..."

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

def rails_modify_model(table_name, table_def, table_keys, is_assoc)
  puts "=> Modify model '" + table_name + "' ..."

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
    @is_association = is_assoc

    @default_def = Hash.new
    if @attrs_def != nil
      @attrs_def.each do |k, v|
        @default_def[keyword(k)] = rails_attr_get_default(@attrs_def, k)
      end
    end

    @ipaddr_keys = table_keys.select {|k, v| v["type"] == "ipv4" || v["type"] == "ipv6"}

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

def rails_api_path(table_name, table_keys)
  path = Array.new
  path << "api"
  path << keyword_plural(table_name)
  path << table_keys.keys.map {|k| ":" + keyword(k)}
  path.join("/")
end

def rails_modify_controller(table_name, table_def, table_keys, is_assoc)
  puts "=> Modify controller '" + table_name + "'..."

  controller_name = keyword_plural(table_name)
  controller = "app/controllers/#{controller_name}_controller.rb"
  template = File.read("../controller.erb")

  File.open(controller, "w") do |f|
    @controller_name = keyword_plural(table_name)
    @parents = table_def["belongs-to"]
    @model_name = keyword(table_name)
    @class_name = keyword_camel(table_name)
    @api_path = rails_api_path(table_name, table_keys)
    @is_association = is_assoc

    @order = nil
    if table_def["order"] != nil
      @order = table_def["order"]
    end

    fields = Hash.new
    if table_keys != nil
      fields.merge!(table_keys)
    end
    if table_def["attributes"] != nil
      fields.merge!(table_def["attributes"])
    end

    keys = Array.new
    if table_def["belongs-to"] != nil
      keys << keyword(table_def["belongs-to"][0]) + "_id"
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
  puts "=> Generate views '" + table_name + "' ..."

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
    keys = table_keys.select {|k, v| v["type"] == "ipv4"}
    if keys.size > 0
      cons = ", constraints: {" +
        keys.map {|k, v| keyword(k) + ': /[\d\.]+/'}.join(', ') + "}"
    end
  end
  
  cons
end

def rails_add_routes(table_name, table_keys)
  puts "=> Add routes '" + table_name + "' ..."

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

def rails_add_tables(options, table2json, table_name, parent_keys_def)
  table_keys = Hash.new
  table_keys.merge!(parent_keys_def) if parent_keys_def != nil
  table_def = table2json[table_name]

  if table_def != nil
    children = table_def["has-dependent"]

    fields = Array.new

    # Association
    if table_def["belongs-to"] != nil
      table_def["belongs-to"].each do |k|
        fields << keyword(k) + "_id:integer"
      end
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
    if !File.exists?(model) and options[:scaffold]
      # Scaffolding
      rails_cmd = "rails generate scaffold " +
        keyword(table_name) + " " + fields.join(" ")
      puts rails_cmd
      system(rails_cmd)
    end

    if options[:migration]
      # Migration: generate index's
      rails_db_add_index(table_name, table_def, table_keys)

      # Migration: set default value
      rails_db_set_default(table_name, table_def)
    end

    # Model: add association
    if options[:model]
      rails_modify_model(table_name, table_def, table_keys, false)
    end

    # Controller: add custom update/destroy methods
    if options[:controller]
      rails_modify_controller(table_name, table_def, table_keys, false)
    end

    # Helper: add get_default
#    if options[:helper]
#      rails_modify_helper(table_name, table_def)
#    end

    # View: generate cli.erb
    if options[:view]
      rails_generate_view(table_name, table_def)
    end

    # Routes: add routes
    if options[:routes]
      rails_add_routes(table_name, table_keys)
    end

    # Iterate children recursively
    if children != nil
      children.each do |child|
        rails_add_tables(options, table2json, child, table_def["keys"])
      end
    end
  end
end

def rails_add_associations(options, table2json, table_name)
  table_keys = Hash.new
  index_keys = Hash.new
  table_def = table2json[table_name]

  if table_def != nil
    fields = Array.new

    # Association, these are keys, too.
    if table_def["belongs-to"] != nil
      table_def["belongs-to"].each do |k|
        key = keyword(k) + "_id"

        fields << key + ":integer"
        table_keys[key] = Hash.new
        table_keys[key]["type"] = "integer"
        if table2json[k] != nil
          index_keys.merge!(table2json[k]["keys"])
        end
      end
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
    if !File.exists?(model) and options[:scaffold]
      # Scaffolding
      rails_cmd = "rails generate scaffold " +
        keyword(table_name) + " " + fields.join(" ")
      puts rails_cmd
      system(rails_cmd)
    end

    if options[:migration]
      # Migration: generate index's
      rails_db_add_index(table_name, table_def, table_keys)

      # Migration: set default value
      rails_db_set_default(table_name, table_def)
    end

    # Model: add association
    if options[:model]
      rails_modify_model(table_name, table_def, table_keys, true)
    end

    # Controller: add custom update/destroy methods
    if options[:controller]
      rails_modify_controller(table_name, table_def, index_keys, true)
    end

    # Helper: add get_default
#    if options[:helper]
#      rails_modify_helper(table_name, table_def)
#    end

    # View: generate cli.erb
    if options[:view]
      rails_generate_view(table_name, table_def)
    end

    # Routes: add routes
    if options[:routes]
      rails_add_routes(table_name, index_keys)
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

# Main
def main(rails_project, dir, options)
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

  # Load *.table.json to get list of parents, association and table2json.
  parents, associations, table2json = rails_load_tables(table_json_dir)

  # Iterate from parents, and then iterate children recursively.
  parents.each do |p|
    rails_add_tables(options, table2json, p, nil)
  end

  # Iterate from associations.
  associations.each do |a|
    rails_add_associations(options, table2json, a)
  end

  # Update mime.types
  rails_update_mime_types if options[:mime] == true

  # rake db:migrate
  system("rake db:migrate")
end

#
# Options
#
options = Hash.new
set_options_all(options)

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
      unset_options_all(options)
      options[:model] = true
      options[:scaffold] = false
    when '--controller-only'
      unset_options_all(options)
      options[:controller] = true
      options[:scaffold] = true
    when '--view-only'
      unset_options_all(options)
      options[:view] = true
      options[:scaffold] = true
  end
end

# Start from here
main(rails_project, dir, options)
