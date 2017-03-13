ALTER TABLE Messages
ADD COLUMN enclosures  TEXT;
-- !
UPDATE Information SET inf_value = '3' WHERE inf_key = 'schema_version';
