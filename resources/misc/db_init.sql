DROP TABLE IF EXISTS Information;
-- !
CREATE TABLE IF NOT EXISTS Information (
  key           TEXT      PRIMARY KEY,
  value         TEXT      NOT NULL
);
-- !
INSERT INTO Information VALUES ('schema_version', '0.0.1');
-- !
DROP TABLE IF EXISTS Categories;
-- !
CREATE TABLE IF NOT EXISTS Categories (
  id             INTEGER  PRIMARY KEY,
  title          TEXT     NOT NULL UNIQUE CHECK(title != ''),
  description    TEXT
  icon           BLOB
);