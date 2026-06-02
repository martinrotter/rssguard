ALTER TABLE Feeds ADD COLUMN open_articles INTEGER(1) NOT NULL DEFAULT 0 CHECK (open_articles >= 0 AND open_articles <= 1);
