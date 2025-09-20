§§0
-- !
CREATE TABLE new_MessageFilters (
  id                  $$,
  name                TEXT        NOT NULL CHECK (name != ''),
  script              TEXT        NOT NULL CHECK (script != ''),
  is_enabled          INTEGER     NOT NULL DEFAULT 1 CHECK (is_enabled >= 0 AND is_enabled <= 1),
  ordr                INTEGER     NOT NULL CHECK (ordr >= 0)
);
-- !
INSERT INTO new_MessageFilters (id, name, script, ordr)
SELECT id, name, script, id FROM MessageFilters;
-- !
DROP TABLE MessageFilters;
-- !
ALTER TABLE new_MessageFilters RENAME TO MessageFilters;
-- !
UPDATE MessageFilters
SET ordr = (
  SELECT COUNT(*)
  FROM MessageFilters ct
  WHERE ct.id < MessageFilters.id
);
-- !
§§1