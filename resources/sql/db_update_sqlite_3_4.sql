CREATE TABLE Accounts (
  id              INTEGER     PRIMARY KEY,
  type            TEXT        NOT NULL
);
-- !
INSERT INTO Accounts (type) VALUES ('std-rss');
-- !
DROP TABLE IF EXISTS FeedsData;
-- !
CREATE TABLE TtRssAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  password        TEXT,
  auth_protected  INTEGER(1)  NOT NULL CHECK (auth_protected >= 0 AND auth_protected <= 1) DEFAULT 0,
  auth_username   TEXT,
  auth_password   TEXT,
  url             TEXT        NOT NULL,
  force_update    INTEGER(1)  NOT NULL CHECK (force_update >= 0 AND force_update <= 1) DEFAULT 0,
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE backup_Messages AS SELECT * FROM Messages;
-- !
DROP TABLE Messages;
-- !
CREATE TABLE Messages (
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
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
INSERT INTO Messages (id, is_read, is_deleted, is_important, feed, title, url, author, date_created, contents, is_pdeleted, enclosures, account_id)
SELECT id, is_read, is_deleted, is_important, feed, title, url, author, date_created, contents, is_pdeleted, enclosures, 1  FROM backup_Messages;
-- !
DROP TABLE backup_Messages;
-- !
CREATE TABLE backup_Feeds AS SELECT * FROM Feeds;
-- !
DROP TABLE Feeds;
-- !
CREATE TABLE Feeds (
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
  update_interval INTEGER     NOT NULL CHECK (update_interval >= 1) DEFAULT 15,
  type            INTEGER,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
INSERT INTO Feeds (id, title, description, date_created, icon, category, encoding, url, protected, username, password, update_type, update_type, type, account_id)
SELECT id, title, description, date_created, icon, category, encoding, url, protected, username, password, update_type, update_type, type, 1 FROM backup_Feeds;
-- !
DROP TABLE backup_Feeds;
-- !
CREATE TABLE backup_Categories AS SELECT * FROM Categories;
-- !
DROP TABLE Categories;
-- !
CREATE TABLE Categories (
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
INSERT INTO Categories (id, parent_id, title, description, date_created, icon, account_id)
SELECT id, parent_id, title, description, date_created, icon, 1 FROM backup_Categories;
-- !
DROP TABLE backup_Categories;
-- !
UPDATE Information SET inf_value = '4' WHERE inf_key = 'schema_version';