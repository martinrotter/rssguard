DROP DATABASE IF EXISTS ##;
-- !
CREATE DATABASE IF NOT EXISTS ## CHARACTER SET utf8 COLLATE utf8_general_ci;
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
INSERT INTO Information VALUES (1, 'schema_version', '4');
-- !
CREATE TABLE IF NOT EXISTS Accounts (
  id              INTEGER     PRIMARY KEY,
  type            TEXT        NOT NULL
);
-- !
INSERT INTO Accounts (type) VALUES ('std-rss');
-- !
CREATE TABLE IF NOT EXISTS TtRssAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  password        TEXT,
  auth_protected  INTEGER(1)  NOT NULL CHECK (auth_protected >= 0 AND auth_protected <= 1) DEFAULT 0,
  auth_username   TEXT,
  auth_password   TEXT,
  url             TEXT        NOT NULL,
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
DROP TABLE IF EXISTS Categories;
-- !
CREATE TABLE IF NOT EXISTS Categories (
  id              INTEGER       AUTO_INCREMENT PRIMARY KEY,
  parent_id       INTEGER       NOT NULL,
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
  url             VARCHAR(100),
  protected       INTEGER(1)    NOT NULL CHECK (protected >= 0 AND protected <= 1),
  username        TEXT,
  password        TEXT,
  update_type     INTEGER(1)    NOT NULL CHECK (update_type >= 0),
  update_interval INTEGER       NOT NULL DEFAULT 15 CHECK (update_interval >= 5),
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
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);