Migrating User Data
===================
RSS Guard automatically migrates all your user data if you upgrade to a newer minor version, for example if you update from `3.7.5` to `3.9.1`.

If you decide to upgrade to a new major version, for example from `3.x.x` to `4.x.x`, then existing user data cannot be used. Major versions declared as non-backwards compatible, so such data transition is not supported.

## Migrate `4.x -> 5.x`
RSS Guard `5.x` is fully backwards compatible with RSS Guard `4.x`. All you need to do is to copy all your user data file into RSS Guard 5 folders.

```{attention}
The opposite migration from `5.x` to `4.x` is however not possible. Manual database edits would have to be made.
```

## Migrate `3.9.2 -> 4.x`
```{danger}
Only proceed if you consider yourself a SQL power user, and you know what you are doing!

Make sure that last RSS Guard `3.x.x` version you used with your data was the latest `3.9.2`.
```

Here is a short DIY manual on how to manually update your `database.db` file to `4.x.x` format. Similar approach can be taken if you use **MariaDB** database backend.

Here are SQLs for [old schema](https://github.com/martinrotter/rssguard/blob/3.9.2/resources/sql/db_init_sqlite.sql) and [new schema](https://github.com/martinrotter/rssguard/blob/4.0.0/resources/sql/db_init_sqlite.sql).

### Converting `*Accounts` tables
In `3.x.x` each plugin/account type had its own table where it kept your login usernames, service URLs etc. In `4.x.x` all plugins share one table `Accounts` and place account-specific data into `custom_data` column. You simply can take all rows from any `*Accounts` table (for example `TtRssAccounts`) and insert them into `Accounts`, keeping all columns their default values, except of `type`, which must have one of these values:
* `std-rss` - For standard list of RSS/ATOM feeds
* `tt-rss` - For Tiny Tiny RSS
* `owncloud` - For Nextcloud News
* `greader` - For all Google Reader API services, including Inoreader
* `feedly` - For Feedly
* `gmail` - For Gmail

Then you need to go to **Edit** dialog of your account in RSS Guard (once you complete this migration guide) and check for all missing login information etc.

Once you add the row to the `Accounts` table, it will be assigned a unique integer `id` value, which is used as a foreign key in other DB tables via `account_id` column.

### Converting `Feeds` table
There are some changes in `Feeds` table:
* `url` column is renamed to `source`
* `source_type`, `post_process`, `encoding`, `type`, `protected`, `username`, `password` columns are removed, and their data is now stored in a JSON-serialized form in a new column `custom_data`. Here is an example of a `custom_data` value:
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

Pay attention to `account_id` column as this column is the ID of your account as stated in the above section.

### Converting `Messages` table
Columns were reordered and other than that new column `score` with sane default value was added. Therefore, you can simply copy your data in a column-to-column mode.

Pay attention to `account_id` column as this column is the ID of your account as stated in the above section.

### Other tables
Other tables like `Labels` or `MessageFilters` are unchanged between these two major RSS Guard versions. But you might need to adjust `account_id` to match DB ID of your account.