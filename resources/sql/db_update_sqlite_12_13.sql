CREATE TABLE backup_ta AS SELECT * FROM TtRssAccounts;
-- !
DROP TABLE TtRssAccounts;
-- !
CREATE TABLE IF NOT EXISTS TtRssAccounts (
  id                  INTEGER,
  username            TEXT        NOT NULL,
  password            TEXT,
  auth_protected      INTEGER(1)  NOT NULL CHECK (auth_protected >= 0 AND auth_protected <= 1) DEFAULT 0,
  auth_username       TEXT,
  auth_password       TEXT,
  url                 TEXT        NOT NULL,
  force_update        INTEGER(1)  NOT NULL CHECK (force_update >= 0 AND force_update <= 1) DEFAULT 0,
  update_only_unread  INTEGER(1)  NOT NULL CHECK (update_only_unread >= 0 AND update_only_unread <= 1) DEFAULT 0,
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
INSERT INTO TtRssAccounts (id, username, password, auth_protected, auth_username, auth_password, url, force_update, update_only_unread)
SELECT id, username, password, auth_protected, auth_username, auth_password, url, force_update, 0  FROM backup_ta;
-- !
DROP TABLE backup_ta;
-- !
UPDATE Information SET inf_value = '13' WHERE inf_key = 'schema_version';