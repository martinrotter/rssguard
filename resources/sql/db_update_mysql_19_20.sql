CREATE TABLE backup_feeds AS SELECT * FROM Feeds;
-- !
DROP TABLE Feeds;
-- !
CREATE TABLE IF NOT EXISTS Feeds (
  id              INTEGER       AUTO_INCREMENT PRIMARY KEY,
  title           TEXT          NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    BIGINT,
  icon            BLOB,
  category        INTEGER       NOT NULL CHECK (category >= -1),
  encoding        TEXT,
  source_type     INTEGER,
  url             VARCHAR(1000),
  post_process    TEXT,
  protected       INTEGER(1)    NOT NULL CHECK (protected >= 0 AND protected <= 1),
  username        TEXT,
  password        TEXT,
  update_type     INTEGER(1)    NOT NULL CHECK (update_type >= 0),
  update_interval INTEGER       NOT NULL DEFAULT 15 CHECK (update_interval >= 1),
  type            INTEGER,
  account_id      INTEGER       NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
INSERT INTO Feeds (id, title, description, date_created, icon, category, encoding, url, protected, username, password, update_type, update_interval, type, account_id, custom_id)
SELECT id, title, description, date_created, icon, category, encoding, url, protected, username, password, update_type, update_interval, type, account_id, custom_id  FROM backup_feeds;
-- !
DROP TABLE backup_feeds;
-- !
UPDATE Information SET inf_value = '20' WHERE inf_key = 'schema_version';