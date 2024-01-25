Cleaning Your Database
======================
RSS Guard is able to work well with huge databases, keeping hundreds of thousands of articles. But, sometimes "well" is not well-enough, right? Big DB files bring RSS Guard into problems sometimes and it is good to regularly remove old, unneeded data from it.

There are two ways of keeping your DB file in good shape.

## Cleaning DB file manually
Go to `Tools -> Cleanup database` dialog where you will see all options. Hit `OK` to process the DB file. Note that option `Optimize database file` is checked, then database engine will try to shrink database file (`database.db`) to smallest size possible, removing all empty areas from it.

## Cleaning DB file automatically
There is better and more configurable way of keeping your DB file slim and fit. This feature can be configured application-wide and individually for each feed.

Go to `Tools -> Settings -> Feeds & articles -> Feed fetching -> Limitting amount of articles in feeds` or select some feed and go to `Edit selected items -> Limitting amount of articles in feeds`.

You can select number of newest articles to keep in your feed. Also, you can instruct the feature to not remove your unread/starred articles.

The cleaning action usually runs everytime your feed is fetched.