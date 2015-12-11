CREATE TABLE IF NOT EXISTS Accounts (
  id              INTEGER     PRIMARY KEY,
  type            TEXT        NOT NULL
);
-- !
INSERT INTO Accounts (type) VALUES ('std-rss');
-- !
DROP TABLE IF EXISTS FeedsData;
-- !
CREATE TABLE IF NOT EXISTS TtRssAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  password        TEXT,
  url             TEXT        NOT NULL,
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
ALTER TABLE Messages
ADD COLUMN account_id  INTEGER  NOT NULL DEFAULT (1);
-- !
ALTER TABLE Feeds
ADD COLUMN account_id  INTEGER  NOT NULL DEFAULT (1);
-- !
ALTER TABLE Categories
ADD COLUMN account_id  INTEGER  NOT NULL DEFAULT (1);
-- !
ALTER TABLE Messages
ADD COLUMN custom_id  TEXT;
-- !
ALTER TABLE Feeds
ADD COLUMN custom_id  TEXT;
-- !
ALTER TABLE Categories
ADD COLUMN custom_id  TEXT;
-- !
CREATE TABLE backup_Messages AS SELECT * FROM Messages;
-- !
DROP TABLE Messages;
-- !
CREATE TABLE Messages (
  id              INTEGER     PRIMARY KEY,
  is_read         INTEGER(1)  NOT NULL CHECK (is_read >= 0 AND is_read <= 1) DEFAULT (0),
  is_deleted      INTEGER(1)  NOT NULL CHECK (is_deleted >= 0 AND is_deleted <= 1) DEFAULT (0),
  is_important    INTEGER(1)  NOT NULL CHECK (is_important >= 0 AND is_important <= 1) DEFAULT (0),
  feed            TEXT        NOT NULL,
  title           TEXT        NOT NULL CHECK (title != ''),
  url             TEXT        NOT NULL,
  author          TEXT,
  date_created    INTEGER     NOT NULL CHECK (date_created != 0),
  contents        TEXT,
  is_pdeleted     INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_pdeleted >= 0 AND is_pdeleted <= 1),
  enclosures      TEXT,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
INSERT INTO Messages SELECT * FROM backup_Messages;
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
  update_interval INTEGER     NOT NULL CHECK (update_interval >= 5) DEFAULT 15,
  type            INTEGER,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
INSERT INTO Feeds SELECT * FROM backup_Feeds;
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
INSERT INTO Categories SELECT * FROM backup_Categories;
-- !
DROP TABLE backup_Categories;
-- !
UPDATE Information SET inf_value = '4' WHERE inf_key = 'schema_version';