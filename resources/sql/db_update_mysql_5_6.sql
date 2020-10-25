USE ##;
-- !
ALTER TABLE Messages
ADD COLUMN custom_hash  TEXT;
-- !
UPDATE Information SET inf_value = '6' WHERE inf_key = 'schema_version';