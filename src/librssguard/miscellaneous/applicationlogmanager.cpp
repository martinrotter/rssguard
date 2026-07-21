// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/applicationlogmanager.h"

#include "definitions/globals.h"
#include "gui/dialogs/formlog.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"

#include <cstdlib>
#include <iostream>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>

namespace {
  QFile* s_fileLog = nullptr;
  bool s_disableDebug = false;
} // namespace

ApplicationLogManager* ApplicationLogManager::s_instance = nullptr;

ApplicationLogManager::ApplicationLogManager(Application* application)
  : QObject(), m_application(application), m_logForm(nullptr) {
  s_instance = this;
}

ApplicationLogManager::~ApplicationLogManager() {
  shutdown();
  s_instance = nullptr;
}

void ApplicationLogManager::updateCliDebugStatus() {
  s_disableDebug = !m_application->cmdParser()->isSet(QSL(CLI_DEBUG_SHORT)) &&
                   m_application->settings()->value(GROUP(General), SETTING(General::DisableDebugOutput)).toBool();
}

void ApplicationLogManager::initializeFileBasedLogging() {
  if (!m_application->cmdParser()->isSet(QSL(CLI_LOG_SHORT))) {
    s_fileLog = nullptr;
    return;
  }

  const QString log_file = m_application->cmdParser()->value(QSL(CLI_LOG_SHORT)).trimmed();

  if (log_file.isEmpty()) {
    const QString automatic_log_folder = m_application->userDataFolder() + QDir::separator() + QSL("logs");
    const QString automatic_log_file = automatic_log_folder + QDir::separator() +
                                       QSL("%1_%2.log")
                                         .arg(QSL(APP_LOW_NAME))
                                         .arg(QDateTime::currentDateTimeUtc().date().weekNumber(), 2, 10, QChar('0'));

    QDir().mkpath(automatic_log_folder);
    s_fileLog = new QFile(automatic_log_file, this);
  }
  else {
    s_fileLog = new QFile(log_file, this);
  }

  if (!s_fileLog->open(QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Append |
                       QIODevice::OpenModeFlag::Unbuffered)) {
    qWarningNN << LOGSEC_CORE << "Cannot open log file" << QUOTE_W_SPACE(log_file) << "for writing.";
  }
}

void ApplicationLogManager::displayLog() {
  if (m_logForm == nullptr) {
    m_logForm = new FormLog(m_application->mainForm());

    connect(this,
            &ApplicationLogManager::sendLogToDialog,
            m_logForm,
            &FormLog::appendLogMessage,
            Qt::ConnectionType::QueuedConnection);
    connect(m_logForm, &FormLog::finished, this, [this]() {
      m_logForm->deleteLater();
      m_logForm = nullptr;
    });
  }

  m_logForm->show();
}

void ApplicationLogManager::shutdown() {
  m_logForm = nullptr;
}

void ApplicationLogManager::performLogging(QtMsgType type, const QMessageLogContext& context, const QString& message) {
#ifndef QT_NO_DEBUG_OUTPUT
  if (s_disableDebug && (type == QtMsgType::QtDebugMsg || type == QtMsgType::QtInfoMsg)) {
    return;
  }

  static const QByteArray new_line = TextFactory::newline().toLocal8Bit();
  const QString console_message = qFormatLogMessage(type, context, message);

  std::cerr << console_message.toStdString() << std::endl;

  if (!QCoreApplication::closingDown() && s_fileLog != nullptr) {
    s_fileLog->write(console_message.toUtf8());
    s_fileLog->write(new_line);
  }

  if (s_instance != nullptr) {
    s_instance->displayLogMessageInDialog(console_message);
  }

  if (type == QtMsgType::QtFatalMsg) {
    std::exit(EXIT_FAILURE);
  }
#else
  Q_UNUSED(type)
  Q_UNUSED(context)
  Q_UNUSED(message)
#endif
}

void ApplicationLogManager::displayLogMessageInDialog(const QString& message) {
  if (m_logForm != nullptr && m_logForm->isVisible()) {
    emit sendLogToDialog(message);
  }
}
