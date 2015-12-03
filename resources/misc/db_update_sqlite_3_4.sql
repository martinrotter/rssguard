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
  id              INTEGER     PRIMARY KEY,
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
ADD COLUMN custom_id  INTEGER;
-- !
ALTER TABLE Feeds
ADD COLUMN custom_id  INTEGER;
-- !
ALTER TABLE Categories
ADD COLUMN custom_id  INTEGER;
-- !
UPDATE Information SET inf_value = '4' WHERE inf_key = 'schema_version';