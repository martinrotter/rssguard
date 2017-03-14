CREATE TABLE IF NOT EXISTS Labels (
  id              INTEGER     PRIMARY KEY,
  account_id      INTEGER     NOT NULL,
  color_fg        TEXT        NOT NULL,
  color_bg        TEXT        NOT NULL,
  title           TEXT        NOT NULL,
  description     TEXT,
  custom_id       TEXT,
  custom_hash     TEXT,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
CREATE TABLE IF NOT EXISTS LabelsInMessages (
  id                  INTEGER     PRIMARY KEY,
  message_custom_id   TEXT        NOT NULL,
  label_custom_id     TEXT        NOT NULL,
  account_id          INTEGER     NOT NULL,
  
  FOREIGN KEY (message_custom_id) REFERENCES Messages (custom_id),
  FOREIGN KEY (label_custom_id) REFERENCES Labels (custom_id),
  FOREIGN KEY (account_id) REFERENCES Accounts (id)
);
-- !
UPDATE Information SET inf_value = '8' WHERE inf_key = 'schema_version';