#
#

exit if ARGV[0] == nil
exit if ARGV[1] == nil

proj = ARGV[0]

name = ARGV[1]
name_p = name + "s"

Dir.chdir(proj)

# Remove assets/javascripts
cmd = "rm app/assets/javascripts/#{name_p}.js.coffee"
puts cmd

# Remove assets/stylesheets
cmd = "rm app/assets/stylesheets/#{name_p}.css.scss"
puts cmd

# Remove controllers
cmd = "rm app/controllers/#{name_p}_controllers.rb"
puts cmd

# Remove helpers
cmd = "rm app/helpers/#{name_p}_helper.rb"
puts cmd

# Remove models
cmd = "rm app/models/#{name}.rb"
puts cmd

# Remove views
cmd = "rm -r app/views/#{name_p}"
puts cmd

# Remove db/migrate
cmd = "rm db/migrate/*_#{name_p}.rb"
puts cmd
