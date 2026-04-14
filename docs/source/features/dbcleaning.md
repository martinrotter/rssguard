Cleaning Your Database
======================
RSS Guard can work well with huge databases containing hundreds of thousands of articles. But sometimes "well" is not good enough, right? Large DB files can occasionally cause problems, so it is a good idea to remove old, unneeded data regularly.

There are two ways to keep your DB file in good shape.

## Cleaning the DB file manually
Go to `Tools -> Cleanup database`, where you will see all available options. Click `OK` to process the DB file. Note that if the `Optimize database file` option is checked, then the database engine will try to shrink the database file (`database.db`) to the smallest size possible by removing all empty areas from it.

## Cleaning the DB file automatically
There is a better and more configurable way to keep your DB file slim and efficient. This feature can be configured application-wide and individually for each feed.

Go to `Tools -> Settings -> Feeds & articles -> Feed fetching -> Limiting amount of articles in feeds`, or select a feed and go to `Edit selected items -> Limiting amount of articles in feeds`.

You can select the number of newest articles to keep in your feed. You can also instruct the feature not to remove your unread or starred articles.

The cleaning action usually runs every time your feed is fetched.
