CREATE TABLE Probes (
  id                  $$,
  name                TEXT        NOT NULL CHECK (name != ''),
  color               VARCHAR(7)  NOT NULL CHECK (color != ''),
  fltr                TEXT        NOT NULL CHECK (fltr != ''), /* Regular expression. */
  account_id          INTEGER     NOT NULL,
  
  FOREIGN KEY (account_id) REFERENCES Accounts (id) ON DELETE CASCADE
);