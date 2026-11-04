#if ENGINE == MARIADB
USE ##;
-- !
#endif

#if ENGINE == SQLITE
CREATE INDEX IF NOT EXISTS idx_Messages6 ON Messages (feed, url);
#elif ENGINE == MARIADB
CREATE INDEX /*M! IF NOT EXISTS */ idx_Messages6 ON Messages (feed, url(128));
#endif
