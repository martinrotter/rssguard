DROP TABLE IF EXISTS Information;
-- !
CREATE TABLE IF NOT EXISTS Information (
  key           TEXT        PRIMARY KEY,
  value         TEXT        NOT NULL
);
-- !
INSERT INTO Information VALUES ('schema_version', '0.0.1');
-- !
DROP TABLE IF EXISTS Categories;
-- !
CREATE TABLE IF NOT EXISTS Categories (
  id             INTEGER    PRIMARY KEY,
  parent_id      INTEGER    NOT NULL,
  title          TEXT       NOT NULL UNIQUE CHECK (title != ''),
  description    TEXT,
  date_created   TEXT       NOT NULL CHECK (date_created != ''),
  icon           BLOB,
  type           INTEGER	NOT NULL,
  
  FOREIGN KEY (parent_id) REFERENCES Categories (id)
);
-- !
DROP TABLE IF EXISTS Feeds;
-- !
CREATE TABLE IF NOT EXISTS Feeds (
  id             INTEGER    PRIMARY KEY,
  title          TEXT       NOT NULL CHECK (title != ''),
  description    TEXT,
  date_created   TEXT       NOT NULL CHECK (date_created != ''),
  icon           BLOB,
  category       INTEGER    NOT NULL CHECK (category >= -1),
  encoding       TEXT       NOT NULL CHECK (encoding != ''),
  url            TEXT       NOT NULL UNIQUE CHECK (url != ''),
  type           INTEGER    NOT NULL
);
-- !
DROP TABLE IF EXISTS Messages;
-- !
CREATE TABLE IF NOT EXISTS Messages (
  id             INTEGER    PRIMARY KEY,
  read           INTEGER(1) NOT NULL CHECK (read >= 0 AND read <= 1) DEFAULT (0),
  deleted        INTEGER(1) NOT NULL CHECK (deleted >= 0 AND deleted <= 1) DEFAULT (0),
  important      INTEGER(1) NOT NULL CHECK (important >= 0 AND important <= 1) DEFAULT (0),
  feed           INTEGER    NOT NULL,
  title          TEXT       NOT NULL CHECK (title != ''),
  url            TEXT,
  author         TEXT,
  date_created   TEXT       NOT NULL CHECK (date_created != ''),
  date_updated   TEXT,
  contents       TEXT,
  
  FOREIGN KEY (feed) REFERENCES Feeds (id)
);