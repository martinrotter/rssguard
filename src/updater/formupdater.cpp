#include "updater/formupdater.h"

#include "definitions/definitions.h"
#include "qtsingleapplication/qtsingleapplication.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScrollBar>


FormUpdater::FormUpdater(QWidget *parent)
  : QMainWindow(parent),
    m_state(NoState),
    m_txtOutput(new QTextEdit(this)),
    m_parsedArguments(QHash<QString, QString>())  {

  m_txtOutput->setFontPointSize(10.0);
  m_txtOutput->setReadOnly(true);
  m_txtOutput->setFocusPolicy(Qt::StrongFocus);
  m_txtOutput->setContextMenuPolicy(Qt::DefaultContextMenu);
  m_txtOutput->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                       Qt::TextSelectableByKeyboard |
                                       Qt::LinksAccessibleByKeyboard |
                                       Qt::LinksAccessibleByMouse);

  setCentralWidget(m_txtOutput);
  setWindowTitle("RSS Guard updater");
  setWindowIcon(QIcon(APP_ICON_PATH));
  moveToCenterAndResize();
}

FormUpdater::~FormUpdater() {
}

void FormUpdater::startUpgrade() {
  printHeading("Welcome to RSS Guard updater");

  printText("Analyzing updater arguments...");

  if (QApplication::arguments().size() != 5) {
    printText("Insufficient arguments passed. Update process cannot proceed.");
    printText("\nPress any key to exit updater...");

    m_state = ExitError;
    return;
  }

  // Obtain parameters.
  QStringList arguments = QApplication::arguments();

  m_parsedArguments["updater_path"] = QDir::toNativeSeparators(qApp->applicationFilePath());
  m_parsedArguments["current_version"] = arguments.at(1);
  m_parsedArguments["next_version"] = arguments.at(2);
  m_parsedArguments["rssguard_executable_path"] = QDir::toNativeSeparators(arguments.at(3));
  m_parsedArguments["rssguard_path"] = QDir::toNativeSeparators(QFileInfo(m_parsedArguments["rssguard_executable_path"]).absolutePath());
  m_parsedArguments["update_file_path"] = QDir::toNativeSeparators(arguments.at(4));
  m_parsedArguments["temp_path"] = QDir::toNativeSeparators(QFileInfo(m_parsedArguments["update_file_path"]).absolutePath());
  m_parsedArguments["output_temp_path"] = m_parsedArguments["temp_path"] + QDir::separator() + APP_LOW_NAME;

  printArguments();

  if (!printUpdateInformation() || !doPreparationCleanup() || !doExtractionAndCopying()) {
    printText("\nPress any key to exit updater...");

    m_state = ExitError;
    return;
  }

  doFinalCleanup();

  if (!QProcess::startDetached(m_parsedArguments["rssguard_executable_path"])) {
    printText("RSS Guard was not started successfully. Start it manually.");
    m_state = ExitError;
  }

  printText("\nPress any key to exit updater...");
  m_state = ExitNormal;
}

void FormUpdater::printArguments() {
  printNewline();
  printHeading("Arguments");

  printText(QString("Updater executable file:\n   -> %1").arg(m_parsedArguments["updater_path"]));
  printText(QString("Application executable file:\n   -> %1").arg(m_parsedArguments["rssguard_executable_path"]));
  printText(QString("Temp folder:\n   -> %1").arg(m_parsedArguments["temp_path"]));
  printText(QString("Application temp folder:\n   -> %1").arg(m_parsedArguments["output_temp_path"]));
}

bool FormUpdater::printUpdateInformation() {
  qApp->processEvents();

  bool update_file_exists = QFile::exists(m_parsedArguments["update_file_path"]);

  printNewline();
  printHeading("Update information");

  printText(QString("Version change:\n   -> %1 --> %2").arg(m_parsedArguments["current_version"], m_parsedArguments["next_version"]));
  printText(QString("Update file:\n   -> %1").arg(m_parsedArguments["update_file_path"]));
  printText(QString("Update file exists:\n   -> %1").arg(update_file_exists ? "yes" : "no"));
  printText(QString("Update file size:\n   -> %1 bytes").arg(QFileInfo(m_parsedArguments["update_file_path"]).size()));

  if (!update_file_exists) {
    printText("\nUpdate file does not exist or is corrupted.");
  }

  return update_file_exists;
}

bool FormUpdater::doPreparationCleanup() {
  qApp->processEvents();

  printNewline();
  printHeading("Initial cleanup");

  // Check if main RSS Guard instance is running.
  if (static_cast<QtSingleApplication*>(qApp)->sendMessage(APP_QUIT_INSTANCE)) {
    printText("The main application is running. Quitting it.");
  }
  else {
    printText("The main application is not running.");
  }

  // Remove old folders.
  if (QDir(m_parsedArguments["output_temp_path"]).exists()) {
    if (!removeDirectory(m_parsedArguments["output_temp_path"])) {
      printText("Cleanup of old temporary files failed.");
      return false;
    }
  }

  if (!removeDirectory(m_parsedArguments["rssguard_path"],
                       QStringList() << APP_7ZA_EXECUTABLE,
                       QStringList() << "data")) {
    printText("Full cleanup of actual RSS Guard installation failed.");
    printText("Some files from old installation may persist.");
  }

  if (!QFile::rename(m_parsedArguments["updater_path"], m_parsedArguments["updater_path"] + ".old")) {
    qDebug("Updater executable was not renamed and it will not be updated.");
  }

  return true;
}

bool FormUpdater::doExtractionAndCopying() {
  qApp->processEvents();

  printNewline();
  printHeading("Extraction of update package");

  QStringList extractor_arguments;
  QProcess process_extractor(this);

  extractor_arguments << "x" << m_parsedArguments["update_file_path"] << "-r" <<
                         "-y" << QString("-o%1").arg(m_parsedArguments["output_temp_path"]);

  process_extractor.setEnvironment(QProcessEnvironment::systemEnvironment().toStringList());
  process_extractor.setWorkingDirectory(m_parsedArguments["rssguard_path"]);
  process_extractor.start(APP_7ZA_EXECUTABLE, extractor_arguments);

  if (!process_extractor.waitForFinished()) {
    process_extractor.close();
    printText("Extraction of update files failed.");
    return false;
  }

  printText(process_extractor.readAll());
  printText("Extraction done successfully.");

  // Find "rssguard" subfolder path in
  QFileInfoList rssguard_temp_root = QDir(m_parsedArguments["output_temp_path"]).entryInfoList(QDir::Dirs |
                                                                                               QDir::NoDotAndDotDot |
                                                                                               QDir::NoSymLinks);

  if (rssguard_temp_root.size() != 1) {
    printText("Could not find root of downloaded application data.");
    return false;
  }

  QString rssguard_single_temp_root = rssguard_temp_root.at(0).absoluteFilePath();

  if (!copyDirectory(rssguard_single_temp_root, m_parsedArguments["rssguard_path"])) {
    printText("Critical error appeared during copying of application files.");
    return false;
  }

  return true;
}

bool FormUpdater::doFinalCleanup() {
  qApp->processEvents();

  printNewline();
  printHeading("Final cleanup");

  return removeDirectory(m_parsedArguments["output_temp_path"]) &&
      QFile::remove(m_parsedArguments["update_file_path"]);
}

void FormUpdater::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::Copy)) {
    event->accept();
    return;
  }
  else {
    event->ignore();
  }

  switch (m_state) {
    case NoState:
      break;

    case ExitNormal:
    case ExitError:
      qApp->quit();
      break;

    default:
      break;
  }
}

void FormUpdater::printHeading(const QString &header) {
  m_txtOutput->setAlignment(Qt::AlignCenter);
  m_txtOutput->append(QString("****** %1 ******\n").arg(header));
}

void FormUpdater::printText(const QString &text) {
  m_txtOutput->setAlignment(Qt::AlignLeft);
  m_txtOutput->append(text);
}

void FormUpdater::printNewline() {
  m_txtOutput->append("\n");
}

void FormUpdater::moveToCenterAndResize() {
  resize(600, 400);
  move(qApp->desktop()->screenGeometry().center() - rect().center());
}

bool FormUpdater::removeDirectory(const QString& directory_name,
                                  const QStringList& exception_file_list,
                                  const QStringList& exception_folder_list) {
  bool result = true;
  QDir dir(directory_name);

  if (dir.exists(directory_name)) {
    foreach (QFileInfo info,
             dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
      if (info.isDir()) {
        if (!exception_folder_list.contains(info.fileName())) {
          result &= removeDirectory(info.absoluteFilePath(), exception_file_list);
        }
      }
      else if (!exception_file_list.contains(info.fileName())) {
        result &= QFile::remove(info.absoluteFilePath());
      }
    }

    result &= dir.rmdir(directory_name);
  }

  return result;
}

bool FormUpdater::copyDirectory(QString source, QString destination) {
  QDir dir(source);

  if (! dir.exists()) {
    return false;
  }

  foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    QString dst_path = destination + QDir::separator() + d;
    dir.mkpath(dst_path);
    copyDirectory(source + QDir::separator() + d, dst_path);
  }

  foreach (QString f, dir.entryList(QDir::Files)) {
    QString original_file = source + QDir::separator() + f;
    QString destination_file = destination + QDir::separator() + f;

    if (!QFile::exists(destination_file) || QFile::remove(destination_file)) {
      if (QFile::copy(original_file, destination_file)) {
        qDebug("Copied file %s", qPrintable(f));
      }
      else {
        qDebug("Failed to copy file %s", qPrintable(original_file));
      }
    }
    else {
      qDebug("Failed to remove file %s", qPrintable(original_file));
    }
  }

  return true;
}
