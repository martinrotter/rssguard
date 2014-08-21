// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "updater/formupdater.h"

#include "definitions/definitions.h"
#include "miscellaneous/iofactory.h"
#include "qtsingleapplication/qtsingleapplication.h"

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
#include <QEventLoop>
#include <QTimer>


FormUpdater *FormUpdater::s_instance;

FormUpdater::FormUpdater(QWidget *parent)
  : QMainWindow(parent, Qt::Dialog | Qt::WindowStaysOnTopHint),
    m_state(NoState),
    m_txtOutput(new QTextEdit(this)),
    m_parsedArguments(QHash<QString, QString>())  {

  // Initialize singleton.
  s_instance = this;

  m_txtOutput->setAutoFormatting(QTextEdit::AutoNone);
  m_txtOutput->setAcceptRichText(true);
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

  connect(this, SIGNAL(debugMessageProduced(QtMsgType,QString)),
          this, SLOT(consumeDebugMessage(QtMsgType,QString)));
}

FormUpdater::~FormUpdater() {
}

void FormUpdater::startUpgrade() {
  qDebug("Started...");

  printHeading("Welcome to RSS Guard updater");
  printText("Analyzing updater arguments.");

  if (QtSingleApplication::arguments().size() != 5) {
    printText("Insufficient arguments passed. Update process cannot proceed.");
    printText("\nPress any key to exit updater...");

    m_state = ExitError;
    return;
  }

  // Process arguments.
  saveArguments();
  printArguments();

  if (!printUpdateInformation() || !doPreparationCleanup() || !doExtractionAndCopying()) {
    printText("\nPress any key to exit updater...");

    m_state = ExitError;
    return;
  }

  doFinalCleanup();
  executeMainApplication();

  printText("\nPress any key to exit updater...");
}

void FormUpdater::saveArguments() {
  // Obtain parameters.
  QStringList arguments = QtSingleApplication::arguments();

  m_parsedArguments["updater_path"] = QDir::toNativeSeparators(qApp->applicationFilePath());
  m_parsedArguments["current_version"] = arguments.at(1);
  m_parsedArguments["next_version"] = arguments.at(2);
  m_parsedArguments["rssguard_executable_path"] = QDir::toNativeSeparators(arguments.at(3));
  m_parsedArguments["rssguard_path"] = QDir::toNativeSeparators(QFileInfo(m_parsedArguments["rssguard_executable_path"]).absolutePath());
  m_parsedArguments["update_file_path"] = QDir::toNativeSeparators(arguments.at(4));
  m_parsedArguments["temp_path"] = QDir::toNativeSeparators(QFileInfo(m_parsedArguments["update_file_path"]).absolutePath());
  m_parsedArguments["output_temp_path"] = m_parsedArguments["temp_path"] + QDir::separator() + APP_LOW_NAME;
}

void FormUpdater::executeMainApplication() {
  printText("\nApplication was upgraded without serious errors.");

  if (!QProcess::startDetached(m_parsedArguments["rssguard_executable_path"])) {
    printText("RSS Guard was not started successfully. Start it manually.");
    m_state = ExitError;
  }
  else {
    m_state = ExitNormal;
  }
}

void FormUpdater::triggerDebugMessageConsumption(QtMsgType type, const QString &message) {
  emit debugMessageProduced(type, message);
}

void FormUpdater::consumeDebugMessage(QtMsgType type, const QString &message) {
  switch (type) {
    case QtDebugMsg:
      printText(message);
      break;

    case QtWarningMsg:
      printText(QString("WARNING: %1").arg(message));
      break;

    case QtCriticalMsg:
      printText(QString("CRITICAL: %1").arg(message));
      break;

    case QtFatalMsg:
      printText(QString("FATAL: %1").arg(message));
      qApp->exit(EXIT_FAILURE);

    default:
      break;
  }
}

#if QT_VERSION >= 0x050000
void FormUpdater::debugHandler(QtMsgType type,
                               const QMessageLogContext &placement,
                               const QString &message) {
#ifndef QT_NO_DEBUG_OUTPUT
  Q_UNUSED(placement)

  s_instance->triggerDebugMessageConsumption(type, message);
#else
  Q_UNUSED(type)
  Q_UNUSED(placement)
  Q_UNUSED(message)
#endif
}
#else
void FormUpdater::debugHandler(QtMsgType type, const char *message) {
#ifndef QT_NO_DEBUG_OUTPUT
  s_instance->triggerDebugMessageConsumption(type, QString(message));
#else
  Q_UNUSED(type)
  Q_UNUSED(message)
#endif
}
#endif

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
  for (int i = 1; i <= 4; i++) {
    qApp->processEvents();

    if (i == 4) {
      printText("Updater made 3 attempts to exit RSS Guard and it failed. Update cannot continue.");
      return false;
    }

    printText(QString("Check for running instances of RSS Guard, attempt %1.").arg(i));

    if (static_cast<QtSingleApplication*>(QCoreApplication::instance())->sendMessage(APP_QUIT_INSTANCE)) {
      printText("The main application is running. Quitting it.");
      printText("Waiting for 6000 ms for main application to finish.");

      QEventLoop blocker(this);
      QTimer::singleShot(6000, &blocker, SLOT(quit()));
      blocker.exec();
    }
    else {
      printText("The main application is not running.");
      break;
    }
  }

  // Remove old folders.
  if (QDir(m_parsedArguments["output_temp_path"]).exists()) {
    if (!IOFactory::removeDirectory(m_parsedArguments["output_temp_path"])) {
      printText("Cleanup of old temporary files failed.");
      return false;
    }
    else {
      printText("Cleanup of old temporary files is done.");
    }
  }

  if (!IOFactory::removeDirectory(m_parsedArguments["rssguard_path"],
                                  QStringList(),
                                  // Keep the folder with settings.
                                  QStringList() << "data")) {
    printText("Full cleanup of actual RSS Guard installation failed.");
    printText("Some files from old installation may persist.");
  }

  return true;
}

bool FormUpdater::doExtractionAndCopying() {
  qApp->processEvents();

  printNewline();
  printHeading("Extraction of update package");

  QStringList extractor_arguments;
  QProcess process_extractor(this);

  extractor_arguments << "x" << "-r" << "-y" <<
                         QString("-o%1").arg(m_parsedArguments["output_temp_path"]) <<
                                                                                       m_parsedArguments["update_file_path"];

  printText(QString("Calling extractor \'%1\' with these arguments:").arg(APP_7ZA_EXECUTABLE));

  foreach(const QString &argument, extractor_arguments) {
    printText(QString("   -> '%1'").arg(argument));
  }

  process_extractor.setEnvironment(QProcessEnvironment::systemEnvironment().toStringList());
  process_extractor.setWorkingDirectory(qApp->applicationDirPath());

  process_extractor.start(APP_7ZA_EXECUTABLE, extractor_arguments);

  if (!process_extractor.waitForFinished()) {
    process_extractor.close();
  }

  printText(process_extractor.readAll());
  printText(QString("Extractor finished with exit code \'%1\'.").arg(process_extractor.exitCode()));

  if (process_extractor.exitCode() != 0 || process_extractor.exitStatus() != QProcess::NormalExit) {
    printText("Extraction failed due errors. Update cannot continue.");
    return false;
  }

  // Find "rssguard" subfolder path in
  QFileInfoList rssguard_temp_root = QDir(m_parsedArguments["output_temp_path"]).entryInfoList(QDir::Dirs |
                                                                                               QDir::NoDotAndDotDot |
                                                                                               QDir::NoSymLinks);

  if (rssguard_temp_root.size() != 1) {
    printText("Could not find root of downloaded application data.");
    return false;
  }

  printNewline();

  QString rssguard_single_temp_root = rssguard_temp_root.at(0).absoluteFilePath();

  if (!IOFactory::copyDirectory(rssguard_single_temp_root, m_parsedArguments["rssguard_path"])) {
    printText("Critical error appeared during copying of application files.");
    return false;
  }

  return true;
}

bool FormUpdater::doFinalCleanup() {
  bool result_file;
  bool result_path;

  qApp->processEvents();

  printNewline();
  printHeading("Final cleanup");

  result_path = IOFactory::removeDirectory(m_parsedArguments["output_temp_path"]);
  result_file = QFile::remove(m_parsedArguments["update_file_path"]);

  printText(QString("Removing temporary files\n   -> %1 -> %2\n   -> %3 -> %4").arg(
              m_parsedArguments["output_temp_path"], result_path ? "success" : "failure",
            m_parsedArguments["update_file_path"], result_file ? "success" : "failure"));

  return result_file && result_path;
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
  m_txtOutput->append("");
}

void FormUpdater::moveToCenterAndResize() {
  resize(600, 400);
  move(qApp->desktop()->screenGeometry().center() - rect().center());
}
