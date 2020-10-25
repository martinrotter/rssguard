USE ##;
-- !
ALTER TABLE OwnCloudAccounts
ADD COLUMN msg_limit INTEGER NOT NULL DEFAULT -1 CHECK (msg_limit >= -1);
-- !
DROP TABLE IF EXISTS Labels;
-- !
DROP TABLE IF EXISTS LabelsInMessages;
-- !
UPDATE Information SET inf_value = '9' WHERE inf_key = 'schema_version';