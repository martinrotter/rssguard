#if ENGINE == MARIADB
USE ##;
-- !
#endif

ALTER TABLE Categories ADD COLUMN custom_data TEXT;
