ALTER TABLE Feeds ADD COLUMN exclude_global_unread INTEGER(1) NOT NULL DEFAULT 0 CHECK (exclude_global_unread >= 0 AND exclude_global_unread <= 1);
