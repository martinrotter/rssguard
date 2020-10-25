USE ##;
-- !
CREATE TABLE IF NOT EXISTS MessageFilters (
  id                  INTEGER     PRIMARY KEY,
  name                TEXT        NOT NULL CHECK (name != ''),
  script              TEXT        NOT NULL CHECK (script != '')
);
-- !
CREATE TABLE IF NOT EXISTS MessageFiltersInFeeds (
  filter                INTEGER     NOT NULL,
  feed_custom_id        TEXT        NOT NULL,
  account_id            INTEGER     NOT NULL,
  
  FOREIGN KEY (filter) REFERENCES MessageFilters (id) ON DELETE CASCADE,
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);
-- !
UPDATE Information SET inf_value = '15' WHERE inf_key = 'schema_version';