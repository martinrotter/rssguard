ALTER TABLE Feeds RENAME TO backup_Feeds;
-- !
CREATE TABLE Feeds (
  id                        $$,
  ordr                      INTEGER     NOT NULL CHECK (ordr >= 0),
  title                     TEXT        NOT NULL CHECK (title != ''),
  description               TEXT,
  date_created              BIGINT,
  icon                      ^^,
  category                  INTEGER     NOT NULL CHECK (category >= -1), /* Physical category ID, also root feeds contain -1 here. */
  source                    TEXT,
  update_type               INTEGER     NOT NULL CHECK (update_type >= 0),
  update_interval           INTEGER     NOT NULL DEFAULT 900 CHECK (update_interval >= 1),
  is_off                    INTEGER     NOT NULL DEFAULT 0 CHECK (is_off >= 0 AND is_off <= 1),
  is_quiet                  INTEGER     NOT NULL DEFAULT 0 CHECK (is_quiet >= 0 AND is_quiet <= 1),
  is_rtl                    INTEGER     NOT NULL DEFAULT 0 CHECK (is_rtl >= 0 AND is_rtl <= 1024),

  add_any_datetime_articles	INTEGER     NOT NULL DEFAULT 0 CHECK (add_any_datetime_articles >= 0 AND add_any_datetime_articles <= 1),
  datetime_to_avoid	        BIGINT      NOT NULL DEFAULT 0 CHECK (datetime_to_avoid >= 0),
  
  keep_article_customize    INTEGER     NOT NULL DEFAULT 0 CHECK (keep_article_customize >= 0 AND keep_article_customize <= 1),
  keep_article_count        INTEGER     NOT NULL DEFAULT 0 CHECK (keep_article_count >= 0),
  keep_unread_articles      INTEGER     NOT NULL DEFAULT 1 CHECK (keep_unread_articles >= 0 AND keep_unread_articles <= 1),
  keep_starred_articles     INTEGER     NOT NULL DEFAULT 1 CHECK (keep_starred_articles >= 0 AND keep_starred_articles <= 1),
  recycle_articles          INTEGER     NOT NULL DEFAULT 0 CHECK (recycle_articles >= 0 AND recycle_articles <= 1),
  account_id                INTEGER     NOT NULL,
  custom_id                 TEXT        NOT NULL CHECK (custom_id != ''), /* Custom ID cannot be empty, it must contain either service-specific ID, or Feeds/id. */
  /* Custom column for (serialized) custom account-specific data. */
  custom_data     TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
INSERT INTO Feeds (id, ordr, title, description, date_created, icon, category, source, update_type, update_interval, is_off, is_quiet, is_rtl, add_any_datetime_articles, datetime_to_avoid, keep_article_customize, keep_article_count, keep_unread_articles, keep_starred_articles, recycle_articles, account_id, custom_id, custom_data)
SELECT id, ordr, title, description, date_created, icon, category, source, update_type, update_interval, is_off, is_quiet, is_rtl, add_any_datetime_articles, datetime_to_avoid, keep_article_customize, keep_article_count, keep_unread_articles, keep_starred_articles, recycle_articles, account_id, custom_id, custom_data
FROM backup_Feeds;
-- !
DROP TABLE backup_Feeds;