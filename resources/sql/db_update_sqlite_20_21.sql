CREATE TABLE IF NOT EXISTS FeedlyAccounts (
  id                        INTEGER,
  username                  TEXT        NOT NULL,
  developer_access_token    TEXT,
  refresh_token             TEXT,
  msg_limit                 INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  update_only_unread        INTEGER(1)  NOT NULL DEFAULT 0 CHECK (update_only_unread >= 0 AND update_only_unread <= 1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
UPDATE Information SET inf_value = '21' WHERE inf_key = 'schema_version';