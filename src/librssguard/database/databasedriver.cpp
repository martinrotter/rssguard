// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasedriver.h"

#include "definitions/definitions.h"
#include "exceptions/ioexception.h"
#include "miscellaneous/iofactory.h"

#include <QDir>
#include <QRegularExpression>

DatabaseDriver::DatabaseDriver(QObject* parent) : QObject(parent)
{}

QStringList DatabaseDriver::prepareScript(const QString& base_sql_folder,
                                          const QString& sql_file,
                                          const QString& database_name) {
  QStringList statements;
  QString next_file = base_sql_folder + QDir::separator() + sql_file;
  QString sql_script = QString::fromUtf8(IOFactory::readFile(next_file));
  QStringList new_statements = sql_script.split(QSL(APP_DB_COMMENT_SPLIT),
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                                Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                                QString::SplitBehavior::SkipEmptyParts);
#endif

  for (int i = 0; i < new_statements.size(); i++) {
    if (new_statements.at(i).startsWith(QSL(APP_DB_INCLUDE_PLACEHOLDER))) {
      // We include another file.
      QString included_file_name = new_statements.at(i).mid(QSL(APP_DB_INCLUDE_PLACEHOLDER).size() + 1);

      QString included_file = base_sql_folder + QDir::separator() + included_file_name;
      QString included_sql_script = QString::fromUtf8(IOFactory::readFile(included_file));
      QStringList included_statements = included_sql_script.split(QSL(APP_DB_COMMENT_SPLIT),
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                                                  Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                                                  QString::SplitBehavior::SkipEmptyParts);
#endif

      statements << included_statements;
    }
    else {
      statements << new_statements.at(i);
    }
  }

  statements.replaceInStrings(QSL(APP_DB_NAME_PLACEHOLDER), database_name);
  statements.replaceInStrings(QSL(APP_DB_AUTO_INC_PRIM_KEY_PLACEHOLDER), autoIncrementPrimaryKey());
  statements.replaceInStrings(QSL(APP_DB_BLOB_PLACEHOLDER), blob());

  return statements;
}
