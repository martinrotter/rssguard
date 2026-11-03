USE ##;
-- !
CREATE INDEX /*M! IF NOT EXISTS */ idx_Messages6 ON Messages (feed, url(128));
