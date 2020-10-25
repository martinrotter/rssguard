USE ##;
-- !
CREATE TABLE IF NOT EXISTS OwnCloudAccounts (
  id              INTEGER,
  username        TEXT        NOT NULL,
  password        TEXT,
  url             TEXT        NOT NULL,
  force_update    INTEGER(1)  NOT NULL DEFAULT 0 CHECK (force_update >= 0 AND force_update <= 1),
  
  FOREIGN KEY (id) REFERENCES Accounts (id)
);
-- !
UPDATE Categories
SET custom_id = (SELECT id FROM Categories t WHERE t.id = Categories.id)
WHERE Categories.custom_id IS NULL OR Categories.custom_id = '';
-- !
UPDATE Feeds
SET custom_id = (SELECT id FROM Feeds t WHERE t.id = Feeds.id)
WHERE Feeds.custom_id IS NULL OR Feeds.custom_id = '';
-- !
UPDATE Messages
SET custom_id = (SELECT id FROM Messages t WHERE t.id = Messages.id)
WHERE Messages.custom_id IS NULL OR Messages.custom_id = '';
-- !
UPDATE Information SET inf_value = '5' WHERE inf_key = 'schema_version';