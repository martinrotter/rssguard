Queries
=============
Article list offers search box to quickly filter displayed articles. If you want to have your search persistent forever, you can create what we call `Query`. You can right click `Queries` item in feed list and following dialog will show:

<img alt="alt-img" src="images/query-dialog.png" width="300px">

You can select name for your query and more importantly the actual search phrase. You need to enter either valid [regular expression](https://learn.microsoft.com/en-us/dotnet/standard/base-types/regular-expression-language-quick-reference) or SQL `WHERE` clause which is compatible with `Messages` [table](https://github.com/martinrotter/rssguard/blob/master/resources/sql/db_init.sql).

Then you confirm the dialog and your search will show in feed list under `Queries` item. If you click it, all matching articles will be shown.

```{attention}
Count of all (or unread) articles matching your query is disabled in feed list for performance reasons. This limitation might be removed in the future.

Also, regular expression filtering in the database is known to be be relatively slow. So if you have many thousands articles, regular expression queries might be slower to load.
```