CREATE TABLE Information (
  inf_key      VARCHAR(128)    NOT NULL UNIQUE CHECK (inf_key != ''), /* Use VARCHAR as MariaDB 10.3 does no support UNIQUE TEXT columns. */
  inf_value    TEXT
);
-- !
CREATE TABLE Accounts (
  id              $$,
  ordr            INTEGER    NOT NULL CHECK (ordr >= 0),
  type            TEXT       NOT NULL CHECK (type != ''), /* ID of the account type. Each account defines its own, for example 'ttrss'. */
  proxy_type      INTEGER    NOT NULL DEFAULT 0 CHECK (proxy_type >= 0),
  proxy_host      TEXT,
  proxy_port      INTEGER,
  proxy_username  TEXT,
  proxy_password  TEXT,
  custom_data     TEXT   /* Custom column for (serialized) custom account-specific data. */
);
-- !
CREATE TABLE Categories (
  id              $$,
  ordr            INTEGER         NOT NULL CHECK (ordr >= 0),
  parent_id       INTEGER         NOT NULL CHECK (parent_id >= -1), /* Root categories contain -1 here. */
  title           VARCHAR(400)    NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    BIGINT,
  icon            ^^,
  account_id      INTEGER         NOT NULL,
  custom_id       TEXT,

  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
CREATE TABLE Feeds (
  id                        $$,
  ordr                      INTEGER         NOT NULL CHECK (ordr >= 0),
  title                     VARCHAR(400)    NOT NULL CHECK (title != ''),
  description               TEXT,
  date_created              BIGINT,
  icon                      ^^,
  category                  INTEGER         NOT NULL CHECK (category >= -1), /* Physical category ID, root feeds contain -1 here. */
  source                    TEXT,
  update_type               INTEGER         NOT NULL CHECK (update_type >= 0),
  update_interval           INTEGER         NOT NULL DEFAULT 900 CHECK (update_interval >= 1),
  is_off                    INTEGER(1)      NOT NULL DEFAULT 0 CHECK (is_off >= 0 AND is_off <= 1),
  is_quiet                  INTEGER(1)      NOT NULL DEFAULT 0 CHECK (is_quiet >= 0 AND is_quiet <= 1),
  is_rtl                    INTEGER         NOT NULL DEFAULT 0 CHECK (is_rtl >= 0 AND is_rtl <= 1024),
  add_any_datetime_articles	INTEGER(1)      NOT NULL DEFAULT 0 CHECK (add_any_datetime_articles >= 0 AND add_any_datetime_articles <= 1),
  datetime_to_avoid	        BIGINT          NOT NULL DEFAULT 0 CHECK (datetime_to_avoid >= 0),
  keep_article_customize    INTEGER(1)      NOT NULL DEFAULT 0 CHECK (keep_article_customize >= 0 AND keep_article_customize <= 1),
  keep_article_count        INTEGER         NOT NULL DEFAULT 0 CHECK (keep_article_count >= 0),
  keep_unread_articles      INTEGER(1)      NOT NULL DEFAULT 1 CHECK (keep_unread_articles >= 0 AND keep_unread_articles <= 1),
  keep_starred_articles     INTEGER(1)      NOT NULL DEFAULT 1 CHECK (keep_starred_articles >= 0 AND keep_starred_articles <= 1),
  recycle_articles          INTEGER(1)      NOT NULL DEFAULT 0 CHECK (recycle_articles >= 0 AND recycle_articles <= 1),
  account_id                INTEGER         NOT NULL,
  custom_id                 VARCHAR(250)    NOT NULL CHECK (custom_id != ''), /* Custom ID cannot be empty, it must contain either service-specific ID, or Feeds/id. */
  custom_data               TEXT, /* Custom column for (serialized) custom account-specific data. */
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
CREATE TABLE Messages (
  id              $$,
  is_read         INTEGER(1)      NOT NULL DEFAULT 0 CHECK (is_read >= 0 AND is_read <= 1),
  is_important    INTEGER(1)      NOT NULL DEFAULT 0 CHECK (is_important >= 0 AND is_important <= 1),
  is_deleted      INTEGER(1)      NOT NULL DEFAULT 0 CHECK (is_deleted >= 0 AND is_deleted <= 1),
  is_pdeleted     INTEGER(1)      NOT NULL DEFAULT 0 CHECK (is_pdeleted >= 0 AND is_pdeleted <= 1),
  feed            INTEGER         NOT NULL,
  title           VARCHAR(500)    NOT NULL CHECK (title != '') ~~,
  url             VARCHAR(1200)   ~~,
  author          VARCHAR(400)    ~~,
  date_created    BIGINT          NOT NULL CHECK (date_created >= 0),
  contents        **,
  enclosures      TEXT,
  score           REAL            NOT NULL DEFAULT 0.0 CHECK (score >= 0.0 AND score <= 100.0),
  account_id      INTEGER         NOT NULL,
  custom_id       VARCHAR(250),
  custom_hash     VARCHAR(100),

  FOREIGN KEY (feed)        REFERENCES Feeds (id)     ON DELETE NO ACTION, /* You need to temporarily disable foreign checks for MariaDB when refreshing feeds from 3rd-party online API, because NO ACTION is synonym for RESTRICT. */
  FOREIGN KEY (account_id)  REFERENCES Accounts (id)  ON DELETE CASCADE
);
-- !
CREATE TABLE MessageFilters (
  id                  $$,
  name                VARCHAR(400)    NOT NULL CHECK (name != ''),
  script              TEXT            NOT NULL CHECK (script != ''),
  is_enabled          INTEGER         NOT NULL DEFAULT 1 CHECK (is_enabled >= 0 AND is_enabled <= 1),
  ordr                INTEGER         NOT NULL CHECK (ordr >= 0)
);
-- !
CREATE TABLE MessageFiltersInFeeds (
  filter      INTEGER         NOT NULL,
  feed        INTEGER         NOT NULL,
  account_id  INTEGER         NOT NULL,
  
  FOREIGN KEY (filter)      REFERENCES MessageFilters (id)  ON DELETE CASCADE,
  FOREIGN KEY (feed)        REFERENCES Feeds (id)           ON DELETE NO ACTION, /* You need to temporarily disable foreign checks for MariaDB when refreshing feeds from 3rd-party online API, because NO ACTION is synonym for RESTRICT. */
  FOREIGN KEY (account_id)  REFERENCES Accounts (id)        ON DELETE CASCADE
);
-- !
CREATE TABLE Labels (
  id                  $$,
  name                VARCHAR(200)    NOT NULL CHECK (name != ''),
  color               VARCHAR(7),
  custom_id           VARCHAR(200),
  account_id          INTEGER         NOT NULL,
  
  UNIQUE (account_id, name),
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
CREATE TABLE LabelsInMessages (
  message     INTEGER       NOT NULL,
  label       INTEGER       NOT NULL,
  account_id  INTEGER       NOT NULL,

  UNIQUE (account_id, message, label),
  FOREIGN KEY (message)     REFERENCES Messages (id)  ON DELETE CASCADE,
  FOREIGN KEY (label)       REFERENCES Labels (id)    ON DELETE NO ACTION, /* You need to temporarily disable foreign checks for MariaDB when refreshing labels, because NO ACTION is synonym for RESTRICT. */
  FOREIGN KEY (account_id)  REFERENCES Accounts (id)  ON DELETE CASCADE
);
-- !
CREATE TABLE Probes (
  id                  $$,
  name                VARCHAR(200)    NOT NULL CHECK (name != ''),
  color               VARCHAR(7),
  fltr                TEXT            NOT NULL CHECK (fltr != ''), /* Regular expression. */
  account_id          INTEGER         NOT NULL,
  
  UNIQUE (account_id, name),
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
-- !
-- !
CREATE INDEX idx_Probes1 ON Probes (account_id);
-- !
-- !
CREATE INDEX idx_Mfif1 ON MessageFiltersInFeeds (account_id);
-- !
CREATE INDEX idx_Mfif2 ON MessageFiltersInFeeds (feed);
-- !
CREATE INDEX idx_Mfif3 ON MessageFiltersInFeeds (filter);
-- !
-- !
CREATE INDEX idx_Labels1 ON Labels (account_id);
-- !
CREATE INDEX idx_Labels2 ON Labels (account_id, custom_id);
-- !
-- !
CREATE INDEX idx_Lim1 ON LabelsInMessages (message);
-- !
CREATE INDEX idx_Lim2 ON LabelsInMessages (label);
-- !
CREATE INDEX idx_Lim3 ON LabelsInMessages (account_id, message);
-- !
-- !
CREATE INDEX idx_Categories1 ON Categories (account_id);
-- !
CREATE INDEX idx_Categories2 ON Categories (parent_id);
-- !
CREATE INDEX idx_Categories3 ON Categories (account_id, ordr);
-- !
-- !
CREATE INDEX idx_Feeds1 ON Feeds (account_id);
-- !
CREATE INDEX idx_Feeds2 ON Feeds (category);
-- !
CREATE INDEX idx_Feeds3 ON Feeds (custom_id);
-- !
CREATE INDEX idx_Feeds4 ON Feeds (account_id, ordr);
-- !
-- !
CREATE INDEX idx_Messages1 ON Messages (account_id);
-- !
CREATE INDEX idx_Messages2 ON Messages (feed);
-- !
CREATE INDEX idx_Messages3 ON Messages (custom_id);
-- !
CREATE INDEX idx_Messages4 ON Messages (feed, is_deleted, is_pdeleted, account_id);
-- !
CREATE INDEX idx_Messages5 ON Messages (is_deleted, is_pdeleted, account_id, feed);
-- !
CREATE INDEX idx_Messages6 ON Messages (is_important, is_deleted, is_pdeleted, account_id);
-- !
CREATE INDEX idx_Messages7 ON Messages (is_read, is_deleted, is_pdeleted, account_id);
-- !
CREATE INDEX idx_Messages8 ON Messages (is_deleted, is_pdeleted, account_id, date_created);