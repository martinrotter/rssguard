ALTER TABLE OwnCloudAccounts
ADD COLUMN msg_limit INTEGER NOT NULL DEFAULT -1 CHECK (msg_limit >= -1);
-- !
UPDATE Information SET inf_value = '9' WHERE inf_key = 'schema_version';