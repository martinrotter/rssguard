USE ##;
-- !
ALTER TABLE Accounts ADD COLUMN proxy_type INTEGER NOT NULL DEFAULT 0 CHECK (proxy_type >= 0);
-- !
ALTER TABLE Accounts ADD COLUMN proxy_host TEXT;
-- !
ALTER TABLE Accounts ADD COLUMN proxy_port INTEGER;
-- !
ALTER TABLE Accounts ADD COLUMN proxy_username TEXT;
-- !
ALTER TABLE Accounts ADD COLUMN proxy_password TEXT;
-- !
UPDATE Information SET inf_value = '18' WHERE inf_key = 'schema_version';