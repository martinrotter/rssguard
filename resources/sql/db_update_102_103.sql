ALTER TABLE Messages ADD COLUMN date_retrieved BIGINT NOT NULL DEFAULT 0 CHECK (date_retrieved >= 0);
-- !
UPDATE Messages SET date_retrieved = date_created;
