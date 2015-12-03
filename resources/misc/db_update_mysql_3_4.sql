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

UPDATE Information SET inf_value = '4' WHERE inf_key = 'schema_version';