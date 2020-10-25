USE ##;
-- !
SET FOREIGN_KEY_CHECKS = 0;
-- !
ALTER TABLE Labels MODIFY id INTEGER AUTO_INCREMENT;
-- !
ALTER TABLE MessageFilters MODIFY id INTEGER AUTO_INCREMENT;
-- !
SET FOREIGN_KEY_CHECKS = 1;
-- !
UPDATE Information SET inf_value = '17' WHERE inf_key = 'schema_version';