CREATE TABLE backup_Messages AS SELECT * FROM Messages;
-- !
DROP TABLE Messages;
-- !
CREATE TABLE Messages (
  id              INTEGER     PRIMARY KEY,
  is_read         INTEGER(1)  NOT NULL CHECK (is_read >= 0 AND is_read <= 1) DEFAULT 0,
  is_deleted      INTEGER(1)  NOT NULL CHECK (is_deleted >= 0 AND is_deleted <= 1) DEFAULT 0,
  is_important    INTEGER(1)  NOT NULL CHECK (is_important >= 0 AND is_important <= 1) DEFAULT 0,
  feed            TEXT        NOT NULL,
  title           TEXT        NOT NULL CHECK (title != ''),
  url             TEXT,
  author          TEXT,
  date_created    INTEGER     NOT NULL CHECK (date_created != 0),
  contents        TEXT,
  is_pdeleted     INTEGER(1)  NOT NULL CHECK (is_pdeleted >= 0 AND is_pdeleted <= 1) DEFAULT 0,
  enclosures      TEXT,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  custom_hash     TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
INSERT INTO Messages (id, is_read, is_deleted, is_important, feed, title, url, author, date_created, contents, is_pdeleted, enclosures, account_id, custom_id)
SELECT id, is_read, is_deleted, is_important, feed, title, url, author, date_created, contents, is_pdeleted, enclosures, account_id, custom_id  FROM backup_Messages;
-- !
DROP TABLE backup_Messages;
-- !
UPDATE Information SET inf_value = '6' WHERE inf_key = 'schema_version';