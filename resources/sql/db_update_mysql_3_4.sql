USE ##;
-- !
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
  auth_protected  INTEGER(1)  NOT NULL DEFAULT 0 CHECK (auth_protected >= 0 AND auth_protected <= 1),
  auth_username   TEXT,
  auth_password   TEXT,
  url             TEXT        NOT NULL,
  force_update    INTEGER(1)  NOT NULL DEFAULT 0 CHECK (force_update >= 0 AND force_update <= 1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
ALTER TABLE Messages
ADD COLUMN account_id  INTEGER  NOT NULL DEFAULT 1;
-- !
ALTER TABLE Messages
ADD COLUMN custom_id  TEXT;
-- !
ALTER TABLE Messages
DROP FOREIGN KEY feed;
-- !
ALTER TABLE Messages
MODIFY feed TEXT NOT NULL;
-- !
ALTER TABLE Messages
MODIFY author TEXT;
-- !
ALTER TABLE Messages
MODIFY url  TEXT;
-- !
ALTER TABLE Feeds
ADD COLUMN account_id  INTEGER  NOT NULL DEFAULT 1;
-- !
ALTER TABLE Feeds
ADD COLUMN custom_id  TEXT;
-- !
ALTER TABLE Feeds
MODIFY date_created BIGINT;
-- !
ALTER TABLE Feeds
MODIFY encoding TEXT;
-- !
ALTER TABLE Feeds
MODIFY url  VARCHAR(100);
-- !
ALTER TABLE Feeds
MODIFY type INTEGER;
-- !
ALTER TABLE Categories
ADD COLUMN account_id  INTEGER  NOT NULL DEFAULT 1;
-- !
ALTER TABLE Categories
ADD COLUMN custom_id  TEXT;
-- !
ALTER TABLE Categories
MODIFY date_created BIGINT;
-- !
UPDATE Information SET inf_value = '4' WHERE inf_key = 'schema_version';