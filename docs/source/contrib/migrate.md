Migrating User Data
===================
RSS Guard automatically migrates all your user data if you upgrade to a newer minor version, for example if you update from `3.7.5` to `3.9.1`.

If you decide to upgrade to a new major version, for example from `3.x.x` to `4.x.x`, then existing user data cannot be used. Major versions are usually mutually incompatible.

## Migrate `4.x -> 5.x`
```{danger}
RSS Guard `5.x` is NOT really compatible with RSS Guard `4.x`. Double-check all your data after the migration. Also note that only classic `RSS/ATOM` feeds are migrated; synchronized services are not. Skins and other user data are not migrated either.
```

There is a migration option displayed when you add a classic `RSS/ATOM` profile or account. So when you start RSS Guard 5 with empty data and are prompted to add your first account, select `RSS/ATOM` and continue until you see the migration dialog.

This migration transfers folders, feeds, articles, article filters, labels, and queries. Article filter feed assignments are not migrated, along with some other things, for technical reasons.

You can also trigger the migration manually:
1. Add a standard RSS/ATOM account or profile via `Accounts -> Add account`.
2. Then the dialog allowing you to migrate will show up.
3. Or you can right-click the newly added account in the feed list and select `Import from RSS Guard 4.x`.

## Migrate `3.9.2 -> 4.x`
```{attention}
Only proceed if you consider yourself a SQL power user and you know what you are doing.

Make sure that the last RSS Guard `3.x.x` version you used with your data was the latest `3.9.2`.
```

Here is a short DIY guide on how to manually update your `database.db` file to the `4.x.x` format. A similar approach can be used if you use the **MariaDB** database backend.

Here are SQL files for the [old schema](https://github.com/martinrotter/rssguard/blob/3.9.2/resources/sql/db_init_sqlite.sql) and the [new schema](https://github.com/martinrotter/rssguard/blob/4.0.0/resources/sql/db_init_sqlite.sql).

### Converting `*Accounts` tables
In `3.x.x`, each plugin or account type had its own table where it stored your login usernames, service URLs, and so on. In `4.x.x`, all plugins share one table, `Accounts`, and place account-specific data into the `custom_data` column. You can take all rows from any `*Accounts` table, for example `TtRssAccounts`, and insert them into `Accounts`, keeping all columns at their default values except for `type`, which must have one of these values:
* `std-rss` - For a standard list of RSS/ATOM feeds
* `tt-rss` - For Tiny Tiny RSS
* `owncloud` - For Nextcloud News
* `greader` - For all Google Reader API services, including Inoreader
* `feedly` - For Feedly
* `gmail` - For Gmail

Then you need to go to the **Edit** dialog of your account in RSS Guard, once you complete this migration guide, and check for any missing login information and similar details.

Once you add the row to the `Accounts` table, it will be assigned a unique integer `id` value, which is used as a foreign key in other DB tables via the `account_id` column.

### Converting `Feeds` table
There are some changes in the `Feeds` table:
* `url` column is renamed to `source`
* `source_type`, `post_process`, `encoding`, `type`, `protected`, `username`, and `password` columns are removed, and their data is now stored in JSON-serialized form in a new `custom_data` column. Here is an example of a `custom_data` value:
    ```json
    {
      "encoding": "UTF-8",
      "password": "AwUujeO2efOgYpX3g1/zoOTp9JULcLTZzwfY",
      "post_process": "",
      "protected": false,
      "source_type": 0,
      "type": 3,
      "username": ""
    }
    ```

Pay attention to the `account_id` column, as this column is the ID of your account as stated in the section above.

### Converting `Messages` table
Columns were reordered, and apart from that a new `score` column with a sane default value was added. Therefore, you can simply copy your data in a column-to-column manner.

Pay attention to the `account_id` column, as this column is the ID of your account as stated in the section above.

### Other tables
Other tables such as `Labels` or `MessageFilters` are unchanged between these two major RSS Guard versions. But you might need to adjust `account_id` to match the DB ID of your account.
