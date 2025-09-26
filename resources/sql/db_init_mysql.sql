DROP DATABASE IF EXISTS ##;
-- !
CREATE DATABASE IF NOT EXISTS ## CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
-- !
USE ##;
-- !
!! db_init.sql
-- !
CREATE INDEX IF NOT EXISTS Messages1 ON Messages ( /* For feed-specific custom ID queries. */
  feed,
  account_id,
  custom_id,
  is_deleted,
  is_pdeleted
);
-- !
CREATE INDEX IF NOT EXISTS Messages2 ON Messages ( /* For title/url queries. */
  feed,
  title,
  account_id
);
-- !
CREATE INDEX IF NOT EXISTS Messages3 ON Messages ( /* For whole account queries. */
  is_deleted,
  is_pdeleted,
  account_id
);
-- !
CREATE INDEX IF NOT EXISTS Messages4 ON Messages ( /* For whole account queries. */
  is_read,
  is_deleted,
  is_pdeleted,
  account_id
);
-- !
CREATE INDEX IF NOT EXISTS Messages5 ON Messages (
  custom_id
);
-- !
CREATE INDEX IF NOT EXISTS Categories1 ON Categories (
  account_id
);
-- !
CREATE INDEX IF NOT EXISTS Feeds1 ON Feeds (
  custom_id
);
-- !
CREATE INDEX IF NOT EXISTS Feeds2 ON Feeds (
  account_id
);
-- !
CREATE INDEX IF NOT EXISTS Labels1 ON Labels (
  custom_id,
  name
);