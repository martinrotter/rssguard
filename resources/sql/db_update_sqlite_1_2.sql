ALTER TABLE Feeds RENAME TO backup_Feeds;
-- !
CREATE TABLE Feeds (
  id              $$,
  title           TEXT        NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    BIGINT,
  icon            ^^,
  category        INTEGER     NOT NULL CHECK (category >= -1), /* Physical category ID, also root feeds contain -1 here. */
  source          TEXT,
  update_type     INTEGER     NOT NULL CHECK (update_type >= 0),
  update_interval INTEGER     NOT NULL DEFAULT 15 CHECK (update_interval >= 1),
  is_off          INTEGER     NOT NULL DEFAULT 0 CHECK (is_off >= 0 AND is_off <= 1),
  open_articles   INTEGER     NOT NULL DEFAULT 0 CHECK (open_articles >= 0 AND open_articles <= 1),
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT        NOT NULL CHECK (custom_id != ''), /* Custom ID cannot be empty, it must contain either service-specific ID, or Feeds/id. */
  /* Custom column for (serialized) custom account-specific data. */
  custom_data     TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
INSERT INTO Feeds (id, title, description, date_created, icon, category, source, update_type, update_interval, account_id, custom_id, custom_data)
SELECT id, title, description, date_created, icon, category, source, update_type, update_interval, account_id, custom_id, custom_data
FROM backup_Feeds;
-- !
DROP TABLE backup_Feeds;