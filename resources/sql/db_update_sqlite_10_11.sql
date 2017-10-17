CREATE TABLE IF NOT EXISTS GmailAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  app_id          TEXT,
  app_key         TEXT,
  redirect_url    TEXT,
  refresh_token   TEXT,
  msg_limit       INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
UPDATE Information SET inf_value = '11' WHERE inf_key = 'schema_version';