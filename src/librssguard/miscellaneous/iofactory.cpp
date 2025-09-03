// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/iofactory.h"

#include "definitions/definitions.h"
#include "exceptions/ioexception.h"
#include "exceptions/processexception.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QTemporaryFile>

IOFactory::IOFactory() = default;

bool IOFactory::isFolderWritable(const QString& folder) {
  QString real_file = folder;

  if (!real_file.endsWith(QDir::separator())) {
    real_file += QDir::separator();
  }

  real_file += QSL("test-permissions-file");
  return QTemporaryFile(real_file).open();
}

QString IOFactory::getSystemFolder(QStandardPaths::StandardLocation location) {
  QStringList locations = QStandardPaths::standardLocations(location);

  return locations.isEmpty() ? QString() : locations.at(0);
}

QString IOFactory::ensureUniqueFilename(const QString& name, const QString& append_format) {
  if (!QFile::exists(name)) {
    return name;
  }

  QString tmp_filename = name;
  int i = 1;

  while (QFile::exists(tmp_filename)) {
    tmp_filename = name;
    const int index = tmp_filename.lastIndexOf(QL1C('.'));
    const QString append_string = append_format.arg(i++);

    if (index < 0) {
      tmp_filename.append(append_string);
    }
    else {
      tmp_filename = tmp_filename.left(index) + append_string + tmp_filename.mid(index);
    }
  }

  return tmp_filename;
}

QString IOFactory::filterBadCharsFromFilename(const QString& name) {
  QString value = name;

  value.replace(QL1C('/'), QL1C('-'));
  value.remove(QL1C('\\'));
  value.remove(QL1C(':'));
  value.remove(QL1C('*'));
  value.remove(QL1C('?'));
  value.remove(QL1C('"'));
  value.remove(QL1C('<'));
  value.remove(QL1C('>'));
  value.remove(QL1C('|'));
  return value;
}

bool IOFactory::startProcessDetached(const QString& executable,
                                     const QStringList& arguments,
                                     const QString& working_directory) {
  QProcess process;

  process.setProgram(executable);
  process.setArguments(arguments);
  process.setWorkingDirectory(working_directory);

  return process.startDetached(nullptr);
}

void IOFactory::startProcess(QProcess* const proc,
                             const QString& executable,
                             const QStringList& arguments,
                             const QString& working_directory,
                             const QProcessEnvironment& pe) {
  proc->setProgram(executable);
  proc->setArguments(arguments);

  QProcessEnvironment system_pe = QProcessEnvironment::systemEnvironment();

  system_pe.insert(pe);
  proc->setProcessEnvironment(system_pe);

  if (!working_directory.isEmpty()) {
    proc->setWorkingDirectory(working_directory);
  }

  proc->start();
}

QString IOFactory::startProcessGetOutput(const QString& executable,
                                         const QStringList& arguments,
                                         const QString& working_directory,
                                         const QProcessEnvironment& pe) {
  QProcess proc;

  proc.setProgram(executable);
  proc.setArguments(arguments);

  QProcessEnvironment system_pe = QProcessEnvironment::systemEnvironment();

  system_pe.insert(pe);
  proc.setProcessEnvironment(system_pe);

  if (!working_directory.isEmpty()) {
    proc.setWorkingDirectory(working_directory);
  }

  proc.start();

  if (proc.waitForFinished() && proc.exitStatus() == QProcess::ExitStatus::NormalExit &&
      proc.exitCode() == EXIT_SUCCESS) {
    return proc.readAllStandardOutput();
  }
  else {
    QString err_output = proc.readAllStandardError().simplified();

    throw ProcessException(proc.exitCode(),
                           proc.exitStatus(),
                           proc.error(),
                           err_output.isEmpty() ? proc.errorString() : err_output);
  }
}

QByteArray IOFactory::readFile(const QString& file_path) {
  QFile input_file(file_path);
  QByteArray input_data;

  if (input_file.open(QIODevice::Text | QIODevice::Unbuffered | QIODevice::ReadOnly)) {
    input_data = input_file.readAll();
    input_file.close();
    return input_data;
  }
  else {
    throw IOException(tr("Cannot open file '%1' for reading.").arg(QDir::toNativeSeparators(file_path)));
  }
}

void IOFactory::debugWriteFile(const QString& file_path, const QByteArray& data) {
#if !defined(NDEBUG)
  writeFile(file_path, data);
#else
  Q_UNUSED(file_path)
  Q_UNUSED(data)
#endif
}

void IOFactory::writeFile(const QString& file_path, const QByteArray& data) {
  QFile input_file(file_path);

  if (input_file.open(QIODevice::WriteOnly)) {
    input_file.write(data);
    input_file.close();
  }
  else {
    throw IOException(tr("Cannot open file '%1' for writting.").arg(QDir::toNativeSeparators(file_path)));
  }
}

bool IOFactory::copyFile(const QString& source, const QString& destination) {
  if (QFile::exists(destination)) {
    QFile file(destination);

    file.setPermissions(file.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser |
                        QFileDevice::WriteGroup | QFileDevice::WriteOther);

    if (!QFile::remove(destination)) {
      return false;
    }
  }

  return QFile::copy(source, destination);
}

void IOFactory::removeFolder(const QString& path) {
  QDir dir(path);
  dir.removeRecursively();
}
