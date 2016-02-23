ALTER TABLE Messages
ADD COLUMN is_pdeleted  INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_pdeleted >= 0 AND is_pdeleted <= 1);
-- !
UPDATE Information SET inf_value = '2' WHERE inf_key = 'schema_version';