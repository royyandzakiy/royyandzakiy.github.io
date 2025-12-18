source "https://rubygems.org"

# THEME: FACTORY RESET
# gem "minima", "~> 2.5"
# gem "jekyll", "~> 4.4.1"

# THEME: TALE - Bundle
# gem "tale"

# THEME: TALE - Github Actions
gem "jekyll-remote-theme"
gem "jekyll-paginate"
gem "jekyll-seo-tag", "~> 2.5"

gem "github-pages", "~> 232", group: :jekyll_plugins # To upgrade, run `bundle update github-pages`.
# disqus: <disqus-id> # this is not the right way to add this

# PLUGINS
group :jekyll_plugins do
  gem "jekyll-feed", "~> 0.12"
  gem "jekyll-youtube", "~> 0.12"
end

platforms :mingw, :x64_mingw, :mswin, :jruby do
  gem "tzinfo", ">= 1", "< 3"
  gem "tzinfo-data"
end

# Performance-booster for watching directories on Windows
gem "wdm", "~> 0.1", :platforms => [:mingw, :x64_mingw, :mswin]
gem "http_parser.rb", "~> 0.6.0", :platforms => [:jruby]