DROP TABLE IF EXISTS Information;
-- !
CREATE TABLE IF NOT EXISTS Information (
  id              INTEGER     PRIMARY KEY,
  inf_key         TEXT        NOT NULL,
  inf_value       TEXT        NOT NULL
);
-- !
INSERT INTO Information VALUES (1, 'schema_version', '0.0.2');
-- !
DROP TABLE IF EXISTS Categories;
-- !
CREATE TABLE IF NOT EXISTS Categories (
  id              INTEGER     PRIMARY KEY,
  parent_id       INTEGER     NOT NULL,
  title           TEXT        NOT NULL UNIQUE CHECK (title != ''),
  description     TEXT,
  date_created    INTEGER     NOT NULL CHECK (date_created != 0),
  icon            BLOB
);
-- !
DROP TABLE IF EXISTS Feeds;
-- !
CREATE TABLE IF NOT EXISTS Feeds (
  id              INTEGER     PRIMARY KEY,
  title           TEXT        NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    INTEGER     NOT NULL CHECK (date_created != 0),
  icon            BLOB,
  category        INTEGER     NOT NULL CHECK (category >= -1),
  encoding        TEXT        NOT NULL CHECK (encoding != ''),
  url             TEXT        NOT NULL UNIQUE CHECK (url != ''),
  protected       INTEGER(1)  NOT NULL CHECK (protected >= 0 AND protected <= 1),
  username        TEXT,
  password        TEXT,
  update_type     INTEGER(1)  NOT NULL CHECK (update_type >= 0),
  update_interval INTEGER     NOT NULL CHECK (update_interval >= 5) DEFAULT 15,
  type            INTEGER     NOT NULL CHECK (type >= 0)
);
-- !
DROP TABLE IF EXISTS FeedsData;
-- !
CREATE TABLE IF NOT EXISTS FeedsData (
  feed_id         INTEGER     NOT NULL,
  feed_key        TEXT        NOT NULL,
  feed_value      TEXT,
  
  PRIMARY KEY (feed_id, feed_key),
  FOREIGN KEY (feed_id) REFERENCES Feeds (id)
);
-- !
DROP TABLE IF EXISTS Messages;
-- !
CREATE TABLE IF NOT EXISTS Messages (
  id              INTEGER     PRIMARY KEY,
  is_read         INTEGER(1)  NOT NULL CHECK (is_read >= 0 AND is_read <= 1) DEFAULT (0),
  is_deleted      INTEGER(1)  NOT NULL CHECK (is_deleted >= 0 AND is_deleted <= 1) DEFAULT (0),
  is_important    INTEGER(1)  NOT NULL CHECK (is_important >= 0 AND is_important <= 1) DEFAULT (0),
  feed            INTEGER     NOT NULL,
  title           TEXT        NOT NULL CHECK (title != ''),
  url             TEXT        NOT NULL,
  author          TEXT        NOT NULL,
  date_created    INTEGER     NOT NULL CHECK (date_created != 0),
  contents        TEXT,
  is_hidden       INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_hidden >= 0 AND is_hidden <= 1),
  
  FOREIGN KEY (feed) REFERENCES Feeds (id)
);