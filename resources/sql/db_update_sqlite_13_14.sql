CREATE TABLE backup_ta AS SELECT * FROM OwnCloudAccounts;
-- !
DROP TABLE OwnCloudAccounts;
-- !
CREATE TABLE IF NOT EXISTS OwnCloudAccounts (
  id                  INTEGER,
  username            TEXT        NOT NULL,
  password            TEXT,
  url                 TEXT        NOT NULL,
  force_update        INTEGER(1)  NOT NULL CHECK (force_update >= 0 AND force_update <= 1) DEFAULT 0,
  msg_limit           INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  update_only_unread  INTEGER(1)  NOT NULL CHECK (update_only_unread >= 0 AND update_only_unread <= 1) DEFAULT 0,
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
INSERT INTO OwnCloudAccounts (id, username, password, url, force_update, msg_limit, update_only_unread)
SELECT id, username, password, url, force_update, msg_limit, 0  FROM backup_ta;
-- !
DROP TABLE backup_ta;
-- !
UPDATE Information SET inf_value = '14' WHERE inf_key = 'schema_version';