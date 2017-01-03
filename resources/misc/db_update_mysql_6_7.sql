-- !
ALTER DATABASE ##
CHARACTER SET = utf8mb4
COLLATE = utf8mb4_unicode_ci;
-- !
USE ##
-- !
ALTER TABLE messages
CONVERT TO CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;
-- !
ALTER TABLE messages
CHANGE title title TEXT
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;
-- !
ALTER TABLE messages
CHANGE contents contents TEXT
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;
-- !
UPDATE Information SET inf_value = '7' WHERE inf_key = 'schema_version';