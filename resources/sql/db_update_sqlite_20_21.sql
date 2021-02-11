CREATE TABLE IF NOT EXISTS FeedlyAccounts (
  id                        INTEGER,
  username                  TEXT        NOT NULL,
  developer_access_token    TEXT,
  refresh_token             TEXT,
  msg_limit                 INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
UPDATE Information SET inf_value = '21' WHERE inf_key = 'schema_version';