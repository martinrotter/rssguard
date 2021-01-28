CREATE TABLE IF NOT EXISTS GoogleReaderApiAccounts (
  id                  INTEGER,
  type                INTEGER     NOT NULL CHECK (type >= 1),
  username            TEXT        NOT NULL,
  password            TEXT,
  url                 TEXT        NOT NULL,
  msg_limit           INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
UPDATE Information SET inf_value = '19' WHERE inf_key = 'schema_version';