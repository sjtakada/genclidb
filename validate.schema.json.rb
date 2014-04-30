#!ruby

require 'rubygems'
require 'json-schema'

if ARGV.count == 2
  puts JSON::Validator.fully_validate(ARGV[0], ARGV[1])
end
