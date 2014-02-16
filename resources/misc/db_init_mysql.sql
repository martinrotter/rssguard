DROP DATABASE IF EXISTS rssguard;
-- !
CREATE DATABASE IF NOT EXISTS rssguard CHARACTER SET utf8 COLLATE utf8_general_ci;
-- !
USE rssguard;
-- !
DROP TABLE IF EXISTS Information;
-- !
CREATE TABLE IF NOT EXISTS Information (
  id              INTEGER     AUTO_INCREMENT PRIMARY KEY,
  inf_key         TEXT        NOT NULL,
  inf_value       TEXT        NOT NULL
);
-- !
INSERT INTO Information VALUES (1, 'schema_version', '0.0.1');
-- !
DROP TABLE IF EXISTS Categories;
-- !
CREATE TABLE IF NOT EXISTS Categories (
  id              INTEGER       AUTO_INCREMENT PRIMARY KEY,
  parent_id       INTEGER       NOT NULL,
  title           VARCHAR(100)  NOT NULL UNIQUE CHECK (title != ''),
  description     TEXT,
  date_created    BIGINT        NOT NULL CHECK (date_created != 0),
  icon            BLOB,
  type            INTEGER       NOT NULL
);
-- !
DROP TABLE IF EXISTS Feeds;
-- !
CREATE TABLE IF NOT EXISTS Feeds (
  id              INTEGER       AUTO_INCREMENT PRIMARY KEY,
  title           TEXT          NOT NULL CHECK (title != ''),
  description     TEXT,
  date_created    BIGINT        NOT NULL CHECK (date_created != 0),
  icon            BLOB,
  category        INTEGER       NOT NULL CHECK (category >= -1),
  encoding        TEXT          NOT NULL CHECK (encoding != ''),
  url             VARCHAR(100)  NOT NULL UNIQUE CHECK (url != ''),
  protected       INTEGER(1)    NOT NULL CHECK (protected >= 0 AND protected <= 1),
  username        TEXT,
  password        TEXT,
  update_type     INTEGER(1)    NOT NULL CHECK (update_type >= 0),
  update_interval INTEGER       NOT NULL DEFAULT 15 CHECK (update_interval >= 5),
  type            INTEGER       NOT NULL CHECK (type >= 0)
);
-- !
DROP TABLE IF EXISTS FeedsData;
-- !
CREATE TABLE IF NOT EXISTS FeedsData (
  feed_id         INTEGER       NOT NULL,
  feed_key        VARCHAR(100)  NOT NULL,
  feed_value      TEXT,
  
  PRIMARY KEY (feed_id, feed_key),
  FOREIGN KEY (feed_id) REFERENCES Feeds (id)
);
-- !
DROP TABLE IF EXISTS Messages;
-- !
CREATE TABLE IF NOT EXISTS Messages (
  id              INTEGER     AUTO_INCREMENT PRIMARY KEY,
  is_read         INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_read >= 0 AND is_read <= 1),
  is_deleted      INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_deleted >= 0 AND is_deleted <= 1),
  is_important    INTEGER(1)  NOT NULL DEFAULT 0 CHECK (is_important >= 0 AND is_important <= 1),
  feed            INTEGER     NOT NULL,
  title           TEXT        NOT NULL CHECK (title != ''),
  url             TEXT,
  author          TEXT,
  date_created    BIGINT      NOT NULL CHECK (date_created != 0),
  contents        TEXT,
  
  FOREIGN KEY (feed) REFERENCES Feeds (id)
);
-- !
INSERT INTO Categories (id, parent_id, title, description, date_created, type, icon) VALUES (1, -1, 'Linux', 'Collections of GNU/Linux-related feeds.', 1388678961000, 0, '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');
-- !
INSERT INTO Categories (id, parent_id, title, description, date_created, type, icon) VALUES (2, -1, 'RSS Guard', 'News and updates on RSS Guard.', 1388678961000, 0, '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');
-- !
INSERT INTO Feeds (title, description, date_created, category, encoding, url, protected, update_type, type) VALUES ('Linux Today', 'Linux Today - Linux News on Internet Time.', 1388678961000, 1, 'UTF-8', 'http://feeds.feedburner.com/linuxtoday/linux?format=xml', 0, 0, 1);
-- !
INSERT INTO Feeds (title, description, date_created, category, encoding, url, protected, update_type, type) VALUES ('LinuxInsider', 'LinuxInsider: Linux News & Information from Around the World.', 1388678961000, 1, 'UTF-8', 'http://www.linuxinsider.com/perl/syndication/rssfull.pl', 0, 0, 2);
-- !
INSERT INTO Feeds (title, description, date_created, category, encoding, url, protected, update_type, type) VALUES ('LXer: Linux News', 'The world is talking about GNU/Linux and Free/Open Source Software.', 1388678961000, 1, 'UTF-8', 'http://lxer.com/module/newswire/headlines.rss', 0, 0, 2);
-- !
INSERT INTO Feeds (title, description, date_created, category, encoding, url, protected, update_type, update_interval, type) VALUES ('Recent Commits', 'Recent commits for RSS Guard project.', 1388678961000, 2, 'UTF-8', 'http://bitbucket.org/skunkos/rssguard/rss', 0, 2, 30, 1);