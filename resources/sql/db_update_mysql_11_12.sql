USE ##;
-- !
ALTER TABLE Feeds
MODIFY url VARCHAR(1000);
-- !
UPDATE Information SET inf_value = '12' WHERE inf_key = 'schema_version';