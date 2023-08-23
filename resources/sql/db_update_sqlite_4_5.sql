/* Add "labels" column. */
ALTER TABLE Messages ADD labels TEXT NOT NULL DEFAULT ".";
-- !
/* Copy label IDs to Messages table. */
UPDATE Messages SET labels = (
  SELECT
    IIF(
      GROUP_CONCAT(LabelsInMessages.label) IS NOT NULL,
      "." || GROUP_CONCAT(LabelsInMessages.label, ".") || ".",
      ".")
    FROM LabelsInMessages
    WHERE Messages.custom_id = LabelsInMessages.message);
-- !
/* Remove LabelsInMessages table. */
DROP TABLE IF EXISTS LabelsInMessages;