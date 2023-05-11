// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef IOFACTORY_H
#define IOFACTORY_H

#include <QCoreApplication>

#include "definitions/definitions.h"

#include <QProcessEnvironment>
#include <QStandardPaths>

class IOFactory {
    Q_DECLARE_TR_FUNCTIONS(IOFactory)

  private:
    IOFactory();

  public:
    static bool isFolderWritable(const QString& folder);

    // Returns system-wide folder according to type.
    static QString getSystemFolder(QStandardPaths::StandardLocation location);

    // Checks given file if it exists and if it does, then generates non-existing new file
    // according to format.
    static QString ensureUniqueFilename(const QString& name, const QString& append_format = QSL("(%1)"));

    // Filters out shit characters from filename.
    static QString filterBadCharsFromFilename(const QString& name);
    static bool startProcessDetached(const QString& executable,
                                     const QStringList& arguments = {},
                                     const QString& working_directory = {});
    static void startProcess(QProcess* const proc,
                             const QString& executable,
                             const QStringList& arguments = {},
                             const QProcessEnvironment& pe = {},
                             const QString& working_directory = {});
    static QString startProcessGetOutput(const QString& executable,
                                         const QStringList& arguments = {},
                                         const QProcessEnvironment& pe = {},
                                         const QString& working_directory = {});

    // Returns contents of a file.
    // Throws exception when no such file exists.
    static QByteArray readFile(const QString& file_path);
    static void writeFile(const QString& file_path, const QByteArray& data);

    // Copies file, overwrites destination.
    static bool copyFile(const QString& source, const QString& destination);

    static void removeFolder(const QString& path);
};

#endif // IOFACTORY_H
