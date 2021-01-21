CREATE TABLE backup_acc AS SELECT * FROM Accounts;
-- !
PRAGMA foreign_keys = OFF;
-- !
DROP TABLE Accounts;
-- !
CREATE TABLE IF NOT EXISTS Accounts (
  id              INTEGER     PRIMARY KEY,
  type            TEXT        NOT NULL CHECK (type != ''),
  proxy_type      INTEGER     NOT NULL CHECK (proxy_type >= 0) DEFAULT 0,
  proxy_host      TEXT,
  proxy_port      INTEGER,
  proxy_username  TEXT,
  proxy_password  TEXT
);
-- !
INSERT INTO Accounts (id, type) SELECT id, type FROM backup_acc;
-- !
DROP TABLE backup_acc;
-- !
PRAGMA foreign_keys = ON;
-- !
UPDATE Information SET inf_value = '18' WHERE inf_key = 'schema_version';