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


# Execute external commands.
def rails_scaffold(table_name, db_fields, force)
  name = keyword(table_name)

  cmd = "rails generate scaffold " + name + " " +
    db_fields.map{|a| a[0] + ":" + a[1] }.join(" ")
  cmd += " --force" if force == true

  puts "=> Scaffolding " + name + " ..."
  puts "# " + cmd

  system cmd
end

def rails_rake_db_migrate_down(table_name)
  cmd = "rake db:migrate:down VERSION=" + $tables[:mig_version][table_name]

  puts "=> DB migrate down " + keyword(table_name) + " ..."
  puts "# " + cmd

  system cmd
end

def rails_rake_db_migrate
  cmd = "rake db:migrate"

  puts "=> Run DB migration ..."
  puts "# " + cmd

  system cmd
end

def rails_migration(table_name)
  name_p = keyword_plural(table_name)
  migration = "add_index_to_" + name_p

  cmd = "rails generate migration " + migration + " --force"

  puts "=> Generate migration to add index to " + name_p + "... "
  puts "# " + cmd

  system cmd

  [name_p, migration]
end


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
  # Generate migration as a place holder.
  name_p, migration = rails_migration(table_name)
  index_keys = table_keys.map{|t| t[0]}
  template = File.read("../add_index.erb")

  Dir.chdir("db/migrate")
  m = Dir.entries(".").select {|f| f =~ /#{migration}\.rb$/}

  if m.size == 1
    migration_file = m[0]

    File.open(migration_file, "w") do |f|
      @class_name = keyword_camel(table_name)
      @index_name = name_p

      # All index keys
      @keys_str = index_keys.map {|k| '"' + keyword(k) + '"'}.join(", ")

      # Generate migration
      renderer = ERB.new(template, nil, '<>')
      f.puts renderer.result()
    end
  else
    puts "Couldn't find migration file: " + migration
  end

  Dir.chdir("../..")
end

def rails_get_migration_create(table_name)
  name = keyword_plural(table_name)
  mig = "create_" + name
  mig_create = nil
  mtime = nil
  version = nil

  if File.exists?("db/migrate")
    Dir.chdir("db/migrate")
    files = Dir.entries(".").select {|f| f =~ /#{mig}\.rb$/}
    
    if files.size == 1
      mig_create = files[0]
      mtime = File.mtime(mig_create)
      m = /^(\d+)_#{mig}\.rb$/.match(mig_create)
      if m != nil
        version = m[1]
      end
    end

    Dir.chdir("../..")
  end

  [mig_create, mtime, version]
end

def rails_list_tables(tables, type)
  tables[type].each do |t|
    if tables[:mig_create][t].nil?
      tables[:flag][t] = "N"
    elsif tables[:mig_mtime][t] < tables[:mtime][t]
      tables[:flag][t] = "U"
    else
      tables[:flag][t] = " "
    end

    version = "N/A"
    version = tables[:mig_version][t]
    printf("%s %-32s %s\n", tables[:flag][t], keyword(t), version)
  end
end

def rails_load_tables(dir)
  tables = Hash.new
  tables[:parents] = Array.new
  tables[:dependents] = Array.new
  tables[:associations] = Array.new
  tables[:json] = Hash.new
  tables[:mtime] = Hash.new
  tables[:mig_create] = Hash.new
  tables[:mig_mtime] = Hash.new
  tables[:mig_version] = Hash.new
  tables[:flag] = Hash.new

  m = Dir.entries(dir).select {|f| f =~ /\.table\.json$/}
  m.each do |f|
    file = dir + "/" + f

    str = File.read(file)
    mtime = File.mtime(file)
    json = JSON.parse(str)

    if json["table"] != nil
      json["table"].each do |table_name, obj|
        if obj["type"] != nil
          if obj["type"] == "parent"
            tables[:parents] << table_name
          elsif obj["type"] == "association"
            tables[:associations] << table_name
          elsif
            tables[:dependents] << table_name
          end
        end

        tables[:json][table_name] = obj
        tables[:mtime][table_name] = mtime

        mig_create, mig_mtime, mig_version =
          rails_get_migration_create(table_name)
        tables[:mig_create][table_name] = mig_create
        tables[:mig_mtime][table_name] = mig_mtime
        tables[:mig_version][table_name] = mig_version
      end
    end
  end

  puts
  puts "parents:"
  rails_list_tables(tables, :parents)

  puts
  puts "dependents:"
  rails_list_tables(tables, :dependents)

  puts
  puts "associations:"
  rails_list_tables(tables, :associations)

  tables
end

def rails_attr_get_default(attributes, key)
  default_value = "nil"

  if attributes != nil
    if attributes[key] != nil
      attr = attributes[key]
      if attr.has_key?("default")
        if attr["type"] == "boolean" or attr["type"] == "integer"
          return attr["default"].to_s
        elsif attr["default"] != nil
          return '"' + attr["default"] + '"'
        end
      end
    end
  end

  default_value
end

def rails_db_set_default(table_name)
  table_def = $table2json[table_name]
  name = keyword_plural(table_name)
  migration = "create_" + name

  Dir.chdir("db/migrate")
  migs = Dir.entries(".").select {|f| f =~ /#{migration}\.rb$/}

  if migs.size == 1
    puts "=> Add default values to '" + keyword(table_name) + "' migrations..."

    migration_file = migs[0]

    lines = Array.new
    File.open(migration_file, "r") do |f|
      while line = f.gets
        m = /^\s+t\.\w+ :(\w+)/.match(line)
        if m != nil
          options = Array.new
          options << m[0]

          options_str = m.post_match
          n = /(limit: \d+)/.match(options_str)
          if n != nil
            options << n[1]
          end

          key = keyword_dashed(m[1])
          default_value = rails_attr_get_default(table_def["attributes"], key)
          if default_value != "nil"
            options << ":null => false"
            options << ":default => " + default_value.to_s
          else
            options << ":null => true"
          end

          line = options.join(", ")
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

def find_by_assoc_keys_statement_str(belongs_to, polymorphic)
  keys = Array.new
  params = Array.new
  conds = Array.new

  if belongs_to != nil
    belongs_to.each do |b|
      keys << keyword(b) + "_id"
      params << keyword(b) + ".id"
      conds << keyword(b)
    end
  end

  if polymorphic != nil
    intf = polymorphic["interface"]
    keys << keyword(intf) + "_type"
    keys << keyword(intf) + "_id"
    params << keyword(intf) + "_type"
    params << keyword(intf) + "_id"
    conds << keyword(intf) + "_type"
    conds << keyword(intf) + "_id"
  end

  cond_str = conds.map {|c| c + " != nil"}.join(" and ")
  func_str = "find_by_" + keys.join("_and_") + "(" + params.join(", ") + ")"

  [func_str, cond_str]
end

def find_by_func_name_str(table_keys)
  "find_by_" + table_keys.map{|t| keyword(t[0])}.join("_and_")
end

def rails_get_belongs_to_assoc(table_def)
  belongs_to = Array.new
  polymorphic = nil

  b = table_def["belongs-to"]
  if !b.nil?
    if !b["table"].nil?
      d = b["table"]
      if d.kind_of?(Array)
        belongs_to = d
      else
        belongs_to = [d]
      end
    end
    if !b["polymorphic"].nil?
      polymorphic = b["polymorphic"]
    end
  end

  [belongs_to, polymorphic]
end

def rails_modify_model(table_name, table_keys)
  puts "=> Modify model '" + keyword(table_name) + "' ..."

  table_def = $table2json[table_name]
  @is_assoc = table_def["type"] == "association"

  @model_name = keyword(table_name)
  model = "app/models/" + @model_name + ".rb"
  template = File.read("../model.erb")

  File.open(model, "w") do |f|
    @class_name = keyword_camel(table_name)
    if !table_def["parent"].nil?
      @belongs_to = [ table_def["parent"] ]
    elsif table_def["type"] == "association"
      @belongs_to, @polymorphic = rails_get_belongs_to_assoc(table_def)
    end
    @find_func_str, @find_cond_str =
      find_by_assoc_keys_statement_str(@belongs_to, @polymorphic)
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
      t[1]["type"] == "ipv4" || t[1]["type"] == "ipv6"
    }

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

def rails_api_path_polymorphic(belongs_to, polymorphic)
  path = Array.new
  path << "api"

  belongs_to.each do |b|
    t = $table2json[b]
    if !t.nil?
      if t["type"] == "dependent" and !t["alias"].nil?
        path << keyword_plural(t["alias"])
      else
        path << keyword_plural(b)
      end

      if !t["keys"].nil?
        t["keys"].each do |k, v|
          path << ":" + keyword(k)
        end
      end
    end
  end

  if polymorphic != nil
    p = polymorphic
    t = $table2json[p]
    if !t.nil?
      path << keyword_plural(p)

      if !t["keys"].nil?
        t["keys"].each do |k, v|
          path << ":" + keyword(k)
        end
      end
    end
  end

  path.join("/")
end

def rails_modify_controller(table_name, table_keys, db_fields, api_path)
  puts "=> Modify controller '" + keyword(table_name) + "'..."

  table_def = $table2json[table_name]
  @is_assoc = table_def["type"] == "association"

  @controller_name = keyword_plural(table_name)
  controller = "app/controllers/#{@controller_name}_controller.rb"
  template = File.read("../controller.erb")

  File.open(controller, "w") do |f|
    @belongs_to, @polymorphic = rails_get_belongs_to_assoc(table_def)
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
  table_def = $table2json[table_name]
  name = keyword(table_name)
  namep = keyword_plural(table_name)
  class_name = keyword_camel(table_name)

  keys = table_def["keys"]
  attrs = table_def["attributes"]

  # Generate only if it doesn't exist
  _view = "app/views/" + namep + "/_index.cli.erb"
  if !File.exists?(_view)
    puts "=> Generate views '" + keyword(table_name) + "' ..."

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

def rails_get_heritage(table_name)
  heritage = Array.new

  table_def = $table2json[table_name]
  if !table_def.nil?
    if !table_def["parent"].nil?
      heritage += rails_get_heritage(table_def["parent"])
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

  # Simple parent association has only parent.
  if table_def["parent"] != nil
    k = table_def["parent"]
    db_fields << [ keyword(k) + "_id", "integer" ]
  end

  # Table keys
  if !table_keys.nil?
    table_keys.each do |tuple|
      key = keyword(tuple[0])
      db_fields << [ key, rails_data_type(tuple[1]) ]
    end
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

def rails_add_routes(table_name, table_keys, api_path)
  puts "=> Add routes for '" + keyword(table_name) + "' ..."

  route = Hash.new
  route[:name] = keyword_plural(table_name)
  route[:path] = api_path
  route[:cons] = rails_keys_constraints(table_keys)
  $routes << route
  $routes_resource << keyword_plural(table_name)
end

def rails_add_polymorphic_routes(table_name_assoc, index_keys,
                                 table_name_polymorphic, api_path)
  puts "=> Add routes for '" + keyword(table_name_assoc) + "' ..."

  cons = rails_keys_constraints(index_keys)
  route = Hash.new
  route[:name] = keyword(table_name_assoc)
  route[:path] = api_path
  route[:polymorphic] = keyword(table_name_polymorphic)
  route[:cons] = cons

  $routes_polymorphic << route
end

def rails_add_table(table_name)
  table_def = $table2json[table_name]
  if table_def.nil?
    puts ">>> Error: No table defintion for " + keyword(table_name)
  else
    # Get list of keys from heritage.
    heritage = rails_get_heritage(table_name)
    table_keys = rails_get_table_keys(heritage)
    db_fields = rails_get_db_fields(table_def, table_keys)
    api_path = rails_api_path(table_name, heritage)

    # New table, scaffold.
    if $tables[:flag][table_name] == "N"
      rails_scaffold(table_name, db_fields, false)
    # Updated table, downgrade first and then scaffold --force
    elsif $tables[:flag][table_name] == "U"
      rails_rake_db_migrate_down(table_name)
      rails_scaffold(table_name, db_fields, true)
    end

    if $tables[:flag][table_name] == "N" or $tables[:flag][table_name] == "U"
      # Migration: generate index's
      rails_db_add_index(table_name, table_keys)

      # Migration: set default value
      rails_db_set_default(table_name)
    end

    # Model: add association
    if $tables[:flag][table_name] == "N" or $tables[:flag][table_name] == "U"
      rails_modify_model(table_name, table_keys)
    end

    # Controller: add custom update/destroy methods
    if $tables[:flag][table_name] == "N" or $tables[:flag][table_name] == "U"
      rails_modify_controller(table_name, table_keys, db_fields, api_path)
    end

    # View: generate cli.erb
    if $tables[:flag][table_name] == "N"
      rails_generate_view(table_name)
    end

    # Routes: add routes
    rails_add_routes(table_name, table_keys, api_path)

    # Iterate children recursively
    children = table_def["has-dependent"]
    if children != nil
      children.each do |child|
        rails_add_table(child)
      end
    end
  end
end

def rails_get_table_keys_assoc(table_def)
  table_keys = Array.new

  belongs_to = table_def["belongs-to"]
  if !belongs_to.nil?
    tables = belongs_to["table"]

    if !tables.nil?
      if tables.kind_of?(Array)
        tables.each do |t|
          k = keyword(t) + "_id"
          v = {"type" => "integer"}
          table_keys << [k, v]
        end
      else
        k = keyword(belongs_to["table"]) + "_id"
        v = {"type" => "integer"}
        table_keys << [k, v]
      end
    end

    polymorphic = belongs_to["polymorphic"]
    if !polymorphic.nil?
      i = polymorphic["interface"]
      k = keyword(i) + "_id"
      v = {"type" => "integer"}
      table_keys << [k, v]
      k = keyword(i) + "_type"
      v = {"type" => "string"}
      table_keys << [k, v]
    end
  end

  table_keys
end

def rails_get_index_keys_assoc(belongs_to, table_name)
  index_keys = Array.new

  belongs_to.each do |t|
    table_def = $table2json[t]
    if !table_def["keys"].nil?
      table_def["keys"].each do |k, v|
        index_keys << [k, v]
      end
    end
  end

  if !table_name.nil?
    table_def = $table2json[table_name]
    if !table_def["keys"].nil?
      table_def["keys"].each do |k, v|
        index_keys << [k, v]
      end
    end
  end

  index_keys
end

def rails_add_association(table_name)
  table_def = $table2json[table_name]
  if table_def.nil?
    puts ">>> Error: No table defintion for " + keyword(table_name)
  else
    table_keys = rails_get_table_keys_assoc(table_def)
    db_fields = rails_get_db_fields(table_def, table_keys)

    # New table, scaffold.
    if $tables[:flag][table_name] == "N"
      rails_scaffold(table_name, db_fields, false)
    # Updated table, downgrade first and then scaffold --force
    elsif $tables[:flag][table_name] == "U"
      rails_rake_db_migrate_down(table_name)
      rails_scaffold(table_name, db_fields, true)
    end

    if $tables[:flag][table_name] == "N" or $tables[:flag][table_name] == "U"
      # Migration: generate index's
      rails_db_add_index(table_name, table_keys)

      # Migration: set default value
      rails_db_set_default(table_name)
    end

    # Model: add association
    if $tables[:flag][table_name] == "N" or $tables[:flag][table_name] == "U"
      rails_modify_model(table_name, table_keys)
    end

    # View: generate cli.erb
    if $tables[:flag][table_name] == "N"
      rails_generate_view(table_name)
    end

    api_path = nil

    # Routes: add routes
    belongs_to, polymorphic = rails_get_belongs_to_assoc(table_def)
    if !polymorphic.nil?
      intf = polymorphic["interface"]
      polymorphic["table"].each do |t|
        api_path = rails_api_path_polymorphic(belongs_to, t)
        index_keys = rails_get_index_keys_assoc(belongs_to, t)
        rails_add_polymorphic_routes(table_name, index_keys, t, api_path)
      end

      api_path = nil
    else
      api_path = rails_api_path_polymorphic(belongs_to, nil)
      index_keys = rails_get_index_keys_assoc(belongs_to, nil)
      rails_add_routes(table_name, index_keys, api_path)
    end

    $routes_resource << keyword_plural(table_name)

    # Controller: add custom update/destroy methods
    if $tables[:flag][table_name] == "N" or $tables[:flag][table_name] == "U"
      rails_modify_controller(table_name, nil, db_fields, api_path)
    end
  end
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

  # Load extra routes.
  @routes_extra = Array.new
  m = Dir.entries("../routes").select {|f| f =~ /\.rb$/}
  m.each do |f|
    file = "../routes/" + f
    str = File.read(file)
    @routes_extra << str
  end

  File.open(routes, "w") do |f|
    @rails_project = $rails_project
    @routes = $routes
    @routes_resource = $routes_resource
    @routes_polymorphic = $routes_polymorphic

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
  $tables = rails_load_tables(table_json_dir)
  $table2json = $tables[:json]

  # Routes.
  $routes = Array.new
  $routes_resource = Array.new
  $routes_polymorphic = Array.new

  # Iterate from parents, and then iterate children recursively.
  $tables[:parents].each do |p|
    rails_add_table(p)
  end

  # Iterate from associations.
  $tables[:associations].each do |a|
    rails_add_association(a)
  end

  # Generate routes
  rails_update_routes

  # Update mime.types
  rails_update_mime_types

  # rake db:migrate
  rails_rake_db_migrate
end


def show_help
  puts "Usage: this-script <rails-project> <table-def-dir> [options...]"
  exit
end

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

#
# Options
#
$options = Hash.new

opts = GetoptLong.new(
  [ '--help',            '-h', GetoptLong::NO_ARGUMENT ]
#  [ '--model-only',      '-m', GetoptLong::NO_ARGUMENT ],
#  [ '--controller-only', '-c', GetoptLong::NO_ARGUMENT ],
#  [ '--view-only',       '-v', GetoptLong::NO_ARGUMENT ]
)

opts.each do |opt, arg|
  case opt
    when '--help'
      show_help
  end
end

#
# Start this script from here.
#
main(db_dir)
