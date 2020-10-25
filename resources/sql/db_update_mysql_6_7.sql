-- !
ALTER DATABASE ##
CHARACTER SET = utf8mb4
COLLATE = utf8mb4_unicode_ci;
-- !
USE ##;
-- !
ALTER TABLE Messages
CONVERT TO CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;
-- !
ALTER TABLE Messages
CHANGE title title TEXT
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;
-- !
ALTER TABLE Messages
CHANGE contents contents TEXT
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;
-- !
UPDATE Information SET inf_value = '7' WHERE inf_key = 'schema_version';