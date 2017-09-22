CREATE TABLE IF NOT EXISTS InoreaderAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  access_token    TEXT,
  refresh_token   TEXT,
  msg_limit       INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
UPDATE Information SET inf_value = '10' WHERE inf_key = 'schema_version';