ALTER TABLE Feeds RENAME TO backup_Feeds;
-- !
CREATE TABLE Feeds (
  id              $$,
  ordr            INTEGER     NOT NULL CHECK (ordr >= 0),
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
INSERT INTO Feeds (id, ordr, title, description, date_created, icon, category, source, update_type, update_interval, account_id, custom_id, custom_data)
SELECT id, id, title, description, date_created, icon, category, source, update_type, update_interval, account_id, custom_id, custom_data
FROM backup_Feeds;
-- !
DROP TABLE backup_Feeds;
-- !
UPDATE Feeds
SET ordr = (
  SELECT COUNT(*)
  FROM Feeds ct
  WHERE Feeds.account_id = ct.account_id AND Feeds.category = ct.category AND ct.id < Feeds.id
);
-- !
ALTER TABLE Categories RENAME TO backup_Categories;
-- !
CREATE TABLE Categories (
  id              $$,
  ordr            INTEGER     NOT NULL CHECK (ordr >= 0),
  parent_id       INTEGER     NOT NULL CHECK (parent_id >= -1), /* Root categories contain -1 here. */
  title           TEXT        NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    BIGINT,
  icon            ^^,
  account_id      INTEGER     NOT NULL,
  custom_id       TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
INSERT INTO Categories (id, ordr, parent_id, title, description, date_created, icon, account_id, custom_id)
SELECT id, id, parent_id, title, description, date_created, icon, account_id, custom_id
FROM backup_Categories;
-- !
DROP TABLE backup_Categories;
-- !
UPDATE Categories
SET ordr = (
  SELECT COUNT(*)
  FROM Categories ct
  WHERE Categories.account_id = ct.account_id AND Categories.parent_id = ct.parent_id AND ct.id < Categories.id
);
-- !
ALTER TABLE Accounts RENAME TO backup_Accounts;
-- !
CREATE TABLE Accounts (
  id              $$,
  ordr            INTEGER     NOT NULL CHECK (ordr >= 0),
  type            TEXT        NOT NULL CHECK (type != ''), /* ID of the account type. Each account defines its own, for example 'ttrss'. */
  proxy_type      INTEGER     NOT NULL DEFAULT 0 CHECK (proxy_type >= 0),
  proxy_host      TEXT,
  proxy_port      INTEGER,
  proxy_username  TEXT,
  proxy_password  TEXT,
  /* Custom column for (serialized) custom account-specific data. */
  custom_data     TEXT
);
-- !
INSERT INTO Accounts (id, ordr, type, proxy_type, proxy_host, proxy_port, proxy_username, proxy_password, custom_data)
SELECT id, id - 1, type, proxy_type, proxy_host, proxy_port, proxy_username, proxy_password, custom_data
FROM backup_Accounts;
-- !
DROP TABLE backup_Accounts;