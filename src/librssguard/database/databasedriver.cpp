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
  auto next_file = base_sql_folder + QDir::separator() + sql_file;
  QString sql_script = QString::fromUtf8(IOFactory::readFile(next_file));
  auto new_statements = sql_script.split(APP_DB_COMMENT_SPLIT,
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                         Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                         QString::SplitBehavior::SkipEmptyParts);
#endif

  for (int i = 0; i < new_statements.size(); i++) {
    if (new_statements.at(i).startsWith(APP_DB_INCLUDE_PLACEHOLDER)) {
      // We include another file.
      QString included_file_name = new_statements.at(i).mid(QSL(APP_DB_INCLUDE_PLACEHOLDER).size() + 1);

      auto included_file = base_sql_folder + QDir::separator() + included_file_name;
      QString included_sql_script = QString::fromUtf8(IOFactory::readFile(included_file));
      auto included_statements = included_sql_script.split(APP_DB_COMMENT_SPLIT,
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

  statements.replaceInStrings(APP_DB_NAME_PLACEHOLDER, database_name);
  statements.replaceInStrings(APP_DB_AUTO_INC_PRIM_KEY_PLACEHOLDER, autoIncrementPrimaryKey());
  return statements;
}
