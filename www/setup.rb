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

# Setup Rails DB environment.
#

require 'json'

#
rails_project = "zebra"
db_dir = "../db"

# Create rails project
# system("rails new #{rails_project}")
Dir.chdir(rails_project)

dir = "../" + db_dir

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

def rails_scaffolding(json)
  json["table"].each do |table_name, table_def|
    children = table_def["children"]

    fields = Array.new
    table_def["keys"].each do |k, obj|
      key = k.gsub(/\-/, "_");
      fields << key + ":" + rails_data_type(obj)
    end

    table_def["attributes"].each do |k, obj|
      key = k.gsub(/\-/, "_");
      fields << key + ":" + rails_data_type(obj)
    end

    str = "rails generate scaffold " + fields.join(" ")
    puts str

#    system("rails generate scaffold")
  end
end

# Iterate table.json files
Dir.entries(dir).each do |f|
  /\.table\.json$/.match(f) do
    str = File.read(dir + "/" + f)
    json = JSON.parse(str)
    rails_scaffolding(json)
  end
end
