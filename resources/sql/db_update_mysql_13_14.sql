USE ##;
-- !
ALTER TABLE OwnCloudAccounts
ADD COLUMN update_only_unread INTEGER(1) NOT NULL DEFAULT 0 CHECK (update_only_unread >= 0 AND update_only_unread <= 1);
-- !
UPDATE Information SET inf_value = '14' WHERE inf_key = 'schema_version';