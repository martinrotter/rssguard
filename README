# 24 Pull Requests

[![Build Status](https://secure.travis-ci.org/andrew/24pullrequests.png?branch=master)](https://travis-ci.org/andrew/24pullrequests) [![Dependency Status](https://gemnasium.com/andrew/24pullrequests.png)](https://gemnasium.com/andrew/24pullrequests) [![Code Climate](https://codeclimate.com/github/andrew/24pullrequests.png)](https://codeclimate.com/github/andrew/24pullrequests) [![Coverage Status](https://coveralls.io/repos/andrew/24pullrequests/badge.png)](https://coveralls.io/r/andrew/24pullrequests)

“Giving back little gifts of code”

24 Pull Requests is a yearly initiative to encourage developers around the world to send a pull request every day in December up to Christmas.

This is the site to help promote the project, highlighting why, how and where to send your pull requests.

## Get started!

* [Explore projects to help](http://24pullrequests.com/projects)
* [Submit your project to get help](http://24pullrequests.com/projects/new)

## Authors

* Andrew Nesbitt
* Chris Lowder
* Baris Balic

## Development

Source hosted at [GitHub](http://github.com/andrew/24pullrequests).
Report issues/feature requests on [GitHub Issues](http://github.com/andrew/24pullrequests/issues). Follow us on Twitter [@24pullrequests](https://twitter.com/24pullrequests).

### Getting Started

New to Ruby? No worries!

First things first, you'll need to install Ruby 2.0. I recommend using the excellent [rbenv](https://github.com/sstephenson/rbenv),
and [ruby-build](https://github.com/sstephenson/ruby-build)

```bash
rbenv install 2.0.0-p353
rbenv global 2.0.0-p353
```

Next, you'll need to make sure that you have postgres installed. This can be
done easily using [Homebrew](http://mxcl.github.com/homebrew/) or by using [http://postgresapp.com](http://postgresapp.com). 

```bash
brew install postgres phantomjs
```

Please see these [further instructions for installing postgres via homebrew](http://www.mikeball.us/blog/setting-up-postgres-with-homebrew/). 

Now, let's install the gems from the `Gemfile` ("Gems" are synonymous with libraries in other
languages).

```bash
gem install bundler && rbenv rehash
bundle install
```

Once all the gems are installed, we'll need to create the databases and
tables. Rails makes this easy through the use of "Rake" tasks.

```bash
bundle exec rake db:create:all
bundle exec rake db:migrate
```

And we can also add some sample data with the **seed** task

```bash
bundle exec rake db:seed
```

If you are working on anything related to the email-generation code, you can use [mailcatcher](https://github.com/sj26/mailcatcher)
Since we use bundler, please read the [following](https://github.com/sj26/mailcatcher#bundler) before using mailcatcher

Almost there! Now all we have to do is start up the Rails server and point
our browser to <http://localhost:3000>

```bash
bundle exec rails s
```

### Tests

Standard RSpec/Capybara tests are used for testing the application. The
tests can be run with `bundle exec rake`.

Mocha/Konacha is used for unit testing any JavaScript. JavaScript specs
should be placed in `spec/javascripts`. Run the JavaScript specs with
`bundle exec rake konacha:serve`.

If you are using the omniauth environment variables
(GITHUB_KEY, GITHUB_SECRET, TWITTER_KEY, TWITTER_SECRET)
for **another** project, you will need to either
 * unset them before running your tests or
 * reset the omniauth environment variables after creating a Github (omniauth) application for this project

as it will use it to learn more about the developers and for pull requests.

### Note on Patches/Pull Requests

 * Fork the project.
 * Make your feature addition or bug fix.
 * Add tests for it. This is important so I don't break it in a future version unintentionally.
 * Send a pull request. Bonus points for topic branches.

## Copyright

Copyright (c) 2013 Andrew Nesbitt. See [LICENSE](https://github.com/andrew/24pullrequests/blob/master/LICENSE) for details.
