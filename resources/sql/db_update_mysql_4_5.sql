USE ##;
-- !
/* Add "labels" column. */
ALTER TABLE Messages ADD labels TEXT NOT NULL DEFAULT ".";
-- !
/* Copy label IDs to Messages table. */
UPDATE Messages SET labels = (
  SELECT
    IF(
      GROUP_CONCAT(LabelsInMessages.label) IS NOT NULL,
      CONCAT(".",REPLACE(GROUP_CONCAT(LabelsInMessages.label), ",", "."), "."),
      ".")
    FROM LabelsInMessages
    WHERE Messages.custom_id = LabelsInMessages.message);
-- !
/* Remove LabelsInMessages table. */
DROP TABLE IF EXISTS LabelsInMessages;