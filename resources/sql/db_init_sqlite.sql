DROP TABLE IF EXISTS Information;
-- !
CREATE TABLE IF NOT EXISTS Information (
  id              INTEGER     PRIMARY KEY,
  inf_key         TEXT        NOT NULL,
  inf_value       TEXT        NOT NULL
);
-- !
INSERT INTO Information VALUES (1, 'schema_version', '17');
-- !
CREATE TABLE IF NOT EXISTS Accounts (
  id              INTEGER     PRIMARY KEY,
  type            TEXT        NOT NULL
);
-- !
CREATE TABLE IF NOT EXISTS TtRssAccounts (
  id                  INTEGER,
  username            TEXT        NOT NULL,
  password            TEXT,
  auth_protected      INTEGER(1)  NOT NULL CHECK (auth_protected >= 0 AND auth_protected <= 1) DEFAULT 0,
  auth_username       TEXT,
  auth_password       TEXT,
  url                 TEXT        NOT NULL,
  force_update        INTEGER(1)  NOT NULL CHECK (force_update >= 0 AND force_update <= 1) DEFAULT 0,
  update_only_unread  INTEGER(1)  NOT NULL CHECK (update_only_unread >= 0 AND update_only_unread <= 1) DEFAULT 0,
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE IF NOT EXISTS OwnCloudAccounts (
  id                  INTEGER,
  username            TEXT        NOT NULL,
  password            TEXT,
  url                 TEXT        NOT NULL,
  force_update        INTEGER(1)  NOT NULL CHECK (force_update >= 0 AND force_update <= 1) DEFAULT 0,
  msg_limit           INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  update_only_unread  INTEGER(1)  NOT NULL CHECK (update_only_unread >= 0 AND update_only_unread <= 1) DEFAULT 0,
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE IF NOT EXISTS InoreaderAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  app_id          TEXT,
  app_key         TEXT,
  redirect_url    TEXT,
  refresh_token   TEXT,
  msg_limit       INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE IF NOT EXISTS GmailAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  app_id          TEXT,
  app_key         TEXT,
  redirect_url    TEXT,
  refresh_token   TEXT,
  msg_limit       INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
DROP TABLE IF EXISTS Categories;
-- !
CREATE TABLE IF NOT EXISTS Categories (
  id              INTEGER     PRIMARY KEY,
  parent_id       INTEGER     NOT NULL,
  title           TEXT        NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    INTEGER,
  icon            BLOB,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
DROP TABLE IF EXISTS Feeds;
-- !
CREATE TABLE IF NOT EXISTS Feeds (
  id              INTEGER     PRIMARY KEY,
  title           TEXT        NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    INTEGER,
  icon            BLOB,
  category        INTEGER     NOT NULL CHECK (category >= -1),
  encoding        TEXT,
  url             TEXT,
  protected       INTEGER(1)  NOT NULL CHECK (protected >= 0 AND protected <= 1),
  username        TEXT,
  password        TEXT,
  update_type     INTEGER(1)  NOT NULL CHECK (update_type >= 0),
  update_interval INTEGER     NOT NULL CHECK (update_interval >= 3) DEFAULT 15,
  type            INTEGER,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
DROP TABLE IF EXISTS Messages;
-- !
CREATE TABLE IF NOT EXISTS Messages (
  id              INTEGER     PRIMARY KEY,
  is_read         INTEGER(1)  NOT NULL CHECK (is_read >= 0 AND is_read <= 1) DEFAULT 0,
  is_deleted      INTEGER(1)  NOT NULL CHECK (is_deleted >= 0 AND is_deleted <= 1) DEFAULT 0,
  is_important    INTEGER(1)  NOT NULL CHECK (is_important >= 0 AND is_important <= 1) DEFAULT 0,
  feed            TEXT        NOT NULL,
  title           TEXT        NOT NULL CHECK (title != ''),
  url             TEXT,
  author          TEXT,
  date_created    INTEGER     NOT NULL CHECK (date_created != 0),
  contents        TEXT,
  is_pdeleted     INTEGER(1)  NOT NULL CHECK (is_pdeleted >= 0 AND is_pdeleted <= 1) DEFAULT 0,
  enclosures      TEXT,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  custom_hash     TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE IF NOT EXISTS MessageFilters (
  id                  INTEGER     PRIMARY KEY,
  name                TEXT        NOT NULL CHECK (name != ''),
  script              TEXT        NOT NULL CHECK (script != '')
);
-- !
CREATE TABLE IF NOT EXISTS MessageFiltersInFeeds (
  filter                INTEGER     NOT NULL,
  feed_custom_id        TEXT        NOT NULL,
  account_id            INTEGER     NOT NULL,
  
  FOREIGN KEY (filter) REFERENCES MessageFilters (id) ON DELETE CASCADE,
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
CREATE TABLE IF NOT EXISTS Labels (
  id                  INTEGER     PRIMARY KEY,
  name                TEXT        NOT NULL CHECK (name != ''),
  color               VARCHAR(7),
  custom_id           TEXT,
  account_id          INTEGER     NOT NULL,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE IF NOT EXISTS LabelsInMessages (
  label             TEXT        NOT NULL, /* Custom ID of label. */
  message           TEXT        NOT NULL, /* Custom ID of message. */
  account_id        INTEGER     NOT NULL,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);