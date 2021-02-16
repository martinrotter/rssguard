DROP DATABASE IF EXISTS ##;
-- !
CREATE DATABASE IF NOT EXISTS ## CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
-- !
USE ##;
-- !
DROP TABLE IF EXISTS Information;
-- !
CREATE TABLE IF NOT EXISTS Information (
  id              INTEGER     AUTO_INCREMENT PRIMARY KEY,
  inf_key         TEXT        NOT NULL,
  inf_value       TEXT        NOT NULL
);
-- !
INSERT INTO Information VALUES (1, 'schema_version', '21');
-- !
CREATE TABLE IF NOT EXISTS Accounts (
  id              INTEGER     AUTO_INCREMENT PRIMARY KEY,
  type            TEXT        NOT NULL CHECK (type != ''),
  proxy_type      INTEGER     NOT NULL DEFAULT 0 CHECK (proxy_type >= 0),
  proxy_host      TEXT,
  proxy_port      INTEGER,
  proxy_username  TEXT,
  proxy_password  TEXT
);
-- !
CREATE TABLE IF NOT EXISTS TtRssAccounts (
  id                    INTEGER,
  username              TEXT        NOT NULL,
  password              TEXT,
  auth_protected        INTEGER(1)  NOT NULL DEFAULT 0 CHECK (auth_protected >= 0 AND auth_protected <= 1),
  auth_username         TEXT,
  auth_password         TEXT,
  url                   TEXT        NOT NULL,
  force_update          INTEGER(1)  NOT NULL DEFAULT 0 CHECK (force_update >= 0 AND force_update <= 1),
  update_only_unread    INTEGER(1)  NOT NULL DEFAULT 0 CHECK (update_only_unread >= 0 AND update_only_unread <= 1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE IF NOT EXISTS OwnCloudAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  password        TEXT,
  url             TEXT        NOT NULL,
  force_update    INTEGER(1)  NOT NULL DEFAULT 0 CHECK (force_update >= 0 AND force_update <= 1),
  msg_limit       INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  update_only_unread    INTEGER(1)  NOT NULL DEFAULT 0 CHECK (update_only_unread >= 0 AND update_only_unread <= 1),
  
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
CREATE TABLE IF NOT EXISTS GoogleReaderApiAccounts (
  id                  INTEGER,
  type                INTEGER     NOT NULL CHECK (type >= 1),
  username            TEXT        NOT NULL,
  password            TEXT,
  url                 TEXT        NOT NULL,
  msg_limit           INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE IF NOT EXISTS FeedlyAccounts (
  id                        INTEGER,
  username                  TEXT        NOT NULL,
  developer_access_token    TEXT,
  refresh_token             TEXT,
  msg_limit                 INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  update_only_unread        INTEGER(1)  NOT NULL DEFAULT 0 CHECK (update_only_unread >= 0 AND update_only_unread <= 1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
DROP TABLE IF EXISTS Categories;
-- !
CREATE TABLE IF NOT EXISTS Categories (
  id              INTEGER       AUTO_INCREMENT PRIMARY KEY,
  parent_id       TEXT          NOT NULL,
  title           VARCHAR(100)  NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    BIGINT,
  icon            BLOB,
  account_id      INTEGER       NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
DROP TABLE IF EXISTS Feeds;
-- !
CREATE TABLE IF NOT EXISTS Feeds (
  id              INTEGER       AUTO_INCREMENT PRIMARY KEY,
  title           TEXT          NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    BIGINT,
  icon            BLOB,
  category        INTEGER       NOT NULL CHECK (category >= -1),
  encoding        TEXT,
  source_type     INTEGER,
  url             VARCHAR(1000),
  post_process    TEXT,
  protected       INTEGER(1)    NOT NULL CHECK (protected >= 0 AND protected <= 1),
  username        TEXT,
  password        TEXT,
  update_type     INTEGER(1)    NOT NULL CHECK (update_type >= 0),
  update_interval INTEGER       NOT NULL DEFAULT 15 CHECK (update_interval >= 1),
  type            INTEGER,
  account_id      INTEGER       NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
DROP TABLE IF EXISTS Messages;
-- !
CREATE TABLE IF NOT EXISTS Messages (
  id              INTEGER     AUTO_INCREMENT PRIMARY KEY,
  is_read         INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_read >= 0 AND is_read <= 1),
  is_deleted      INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_deleted >= 0 AND is_deleted <= 1),
  is_important    INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_important >= 0 AND is_important <= 1),
  feed            TEXT        NOT NULL,
  title           TEXT        NOT NULL CHECK (title != ''),
  url             TEXT,
  author          TEXT,
  date_created    BIGINT      NOT NULL CHECK (date_created != 0),
  contents        TEXT,
  is_pdeleted     INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_pdeleted >= 0 AND is_pdeleted <= 1),
  enclosures      TEXT,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  custom_hash     TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE IF NOT EXISTS MessageFilters (
  id                  INTEGER     AUTO_INCREMENT PRIMARY KEY,
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
  id                  INTEGER     AUTO_INCREMENT PRIMARY KEY,
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