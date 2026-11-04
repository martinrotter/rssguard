#if ENGINE == MARIADB
USE ##;
-- !
#endif

#if ENGINE == SQLITE
DROP INDEX idx_Messages3;
#elif ENGINE == MARIADB
DROP INDEX idx_Messages3 ON Messages;
#endif
-- !
CREATE INDEX idx_Messages3 ON Messages (feed, is_deleted, is_pdeleted, is_read);
