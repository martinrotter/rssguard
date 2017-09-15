CREATE TABLE backup_oa AS SELECT * FROM OwnCloudAccounts;
-- !
DROP TABLE OwnCloudAccounts;
-- !
CREATE TABLE IF NOT EXISTS OwnCloudAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  password        TEXT,
  url             TEXT        NOT NULL,
  force_update    INTEGER(1)  NOT NULL CHECK (force_update >= 0 AND force_update <= 1) DEFAULT 0,
  msg_limit       INTEGER     NOT NULL DEFAULT -1 CHECK (msg_limit >= -1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
INSERT INTO OwnCloudAccounts (id, username, password, url, force_update)
SELECT id, username, password, url, force_update  FROM backup_oa;
-- !
DROP TABLE backup_oa;
-- !
DROP TABLE IF EXISTS Labels;
-- !
DROP TABLE IF EXISTS LabelsInMessages;
-- !
UPDATE Information SET inf_value = '9' WHERE inf_key = 'schema_version';