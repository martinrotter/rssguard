INSERT INTO Information (inf_key, inf_value) VALUES ('standard_account_enabled', 1);
-- !
DROP TABLE IF EXISTS FeedsData;
-- !
UPDATE Information SET inf_value = '4' WHERE inf_key = 'schema_version';