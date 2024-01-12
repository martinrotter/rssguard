CREATE TABLE Information (
  inf_key         VARCHAR(128)    NOT NULL UNIQUE CHECK (inf_key != ''), /* Use VARCHAR as MariaDB 10.3 does no support UNIQUE TEXT columns. */
  inf_value       TEXT
);
-- !
CREATE TABLE Accounts (
  id              $$,
  ordr            INTEGER     NOT NULL CHECK (ordr >= 0),
  type            TEXT        NOT NULL CHECK (type != ''), /* ID of the account type. Each account defines its own, for example 'ttrss'. */
  proxy_type      INTEGER     NOT NULL DEFAULT 0 CHECK (proxy_type >= 0),
  proxy_host      TEXT,
  proxy_port      INTEGER,
  proxy_username  TEXT,
  proxy_password  TEXT,
  /* Custom column for (serialized) custom account-specific data. */
  custom_data     TEXT
);
-- !
CREATE TABLE Categories (
  id              $$,
  ordr            INTEGER     NOT NULL CHECK (ordr >= 0),
  parent_id       INTEGER     NOT NULL CHECK (parent_id >= -1), /* Root categories contain -1 here. */
  title           TEXT        NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    BIGINT,
  icon            ^^,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
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
  is_rtl                    INTEGER     NOT NULL DEFAULT 0 CHECK (is_rtl >= 0 AND is_rtl <= 1),

  add_any_datetime_articles	INTEGER     NOT NULL DEFAULT 0 CHECK (add_any_datetime_articles >= 0 AND add_any_datetime_articles <= 1),
  datetime_to_avoid	        BIGINT      NOT NULL DEFAULT 0 CHECK (datetime_to_avoid >= 0),
  
  keep_article_count        INTEGER     NOT NULL DEFAULT 0 CHECK (keep_article_count >= 0),
  keep_unread_articles      INTEGER     NOT NULL DEFAULT 1 CHECK (keep_unread_articles >= 0 AND keep_unread_articles <= 1),
  keep_starred_articles     INTEGER     NOT NULL DEFAULT 1 CHECK (keep_starred_articles >= 0 AND keep_starred_articles <= 1),
  recycle_articles          INTEGER     NOT NULL DEFAULT 0 CHECK (recycle_articles >= 0 AND recycle_articles <= 1),
  
  open_articles             INTEGER     NOT NULL DEFAULT 0 CHECK (open_articles >= 0 AND open_articles <= 1),
  account_id                INTEGER     NOT NULL,
  custom_id                 TEXT        NOT NULL CHECK (custom_id != ''), /* Custom ID cannot be empty, it must contain either service-specific ID, or Feeds/id. */
  /* Custom column for (serialized) custom account-specific data. */
  custom_data     TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
CREATE TABLE Messages (
  id              $$,
  is_read         INTEGER     NOT NULL DEFAULT 0 CHECK (is_read >= 0 AND is_read <= 1),
  is_important    INTEGER     NOT NULL DEFAULT 0 CHECK (is_important >= 0 AND is_important <= 1),
  is_deleted      INTEGER     NOT NULL DEFAULT 0 CHECK (is_deleted >= 0 AND is_deleted <= 1),
  is_pdeleted     INTEGER     NOT NULL DEFAULT 0 CHECK (is_pdeleted >= 0 AND is_pdeleted <= 1),
  feed            TEXT        NOT NULL, /* Points to Feeds/custom_id. */
  title           TEXT        NOT NULL CHECK (title != ''),
  url             TEXT,
  author          TEXT,
  date_created    BIGINT      NOT NULL CHECK (date_created >= 0),
  contents        TEXT,
  enclosures      TEXT,
  score           REAL        NOT NULL DEFAULT 0.0 CHECK (score >= 0.0 AND score <= 100.0),
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  custom_hash     TEXT,
  labels          TEXT        NOT NULL DEFAULT ".", /* Holds list of assigned label IDs. */
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
CREATE TABLE MessageFilters (
  id                  $$,
  name                TEXT        NOT NULL CHECK (name != ''),
  script              TEXT        NOT NULL CHECK (script != '')
);
-- !
CREATE TABLE MessageFiltersInFeeds (
  filter                INTEGER     NOT NULL,
  feed_custom_id        TEXT        NOT NULL,  /* Points to Feeds/custom_id. */
  account_id            INTEGER     NOT NULL,
  
  FOREIGN KEY (filter) REFERENCES MessageFilters (id) ON DELETE CASCADE,
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
CREATE TABLE Labels (
  id                  $$,
  name                TEXT        NOT NULL CHECK (name != ''),
  color               VARCHAR(7),
  custom_id           TEXT,
  account_id          INTEGER     NOT NULL,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
CREATE TABLE Probes (
  id                  $$,
  name                TEXT        NOT NULL CHECK (name != ''),
  color               VARCHAR(7)  NOT NULL CHECK (color != ''),
  fltr                TEXT        NOT NULL CHECK (fltr != ''), /* Regular expression. */
  account_id          INTEGER     NOT NULL,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);