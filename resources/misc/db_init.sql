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
  type           INTEGER    NOT NULL,
  
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
  language       TEXT,
  type           INTEGER    NOT NULL CHECK (type >= 0)
);
-- !
DROP TABLE IF EXISTS FeedsData;
-- !
CREATE TABLE IF NOT EXISTS FeedsData (
  id             INTEGER	NOT NULL,
  key		     TEXT		NOT NULL,
  value	         TEXT,
  
  PRIMARY KEY (id, key),
  FOREIGN KEY (id) REFERENCES Feeds (id)
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
-- !
INSERT INTO Feeds (title, date_created, category, encoding, url, type) VALUES ('qqq', '1994-11-05T08:15:30-05:00', 0, 'UTF-8', 'http://www.seznam2.cz', 1); 
INSERT INTO Feeds (title, date_created, category, encoding, url, type) VALUES ('vwvw', '1994-11-05T08:15:30-05:00', 1, 'UTF-8', 'http://www.seznam3.cz', 1); 
INSERT INTO Feeds (title, date_created, category, encoding, url, type) VALUES ('cbcd', '1994-11-05T08:15:30-05:00', -1, 'UTF-8', 'http://www.seznam1.cz', 1);
INSERT INTO Feeds (title, date_created, category, encoding, url, type) VALUES ('attt', '1994-11-05T08:15:30-05:00', 1, 'UTF-8', 'http://www.seznam4.cz', 0);
INSERT INTO Feeds (title, date_created, category, encoding, url, type) VALUES ('ahht', '1994-11-05T08:15:30-05:00', -1, 'UTF-8', 'http://www.seznam6.cz', 0);
INSERT INTO Categories (id, parent_id, title, date_created, type) VALUES (1, 0, 'bbb', '1994-11-05T08:15:30-05:00', 0);
INSERT INTO Categories (id, parent_id, title, date_created, type) VALUES (0, -1, 'aaa', '1994-11-05T08:15:30-05:00', 0);
INSERT INTO Categories (id, parent_id, title, date_created, type) VALUES (2, 0, 'ccc', '1994-11-05T08:15:30-05:00', 0);