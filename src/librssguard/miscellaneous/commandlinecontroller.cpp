// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/commandlinecontroller.h"

#include "core/feedsmodel.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/application.h"
#include "miscellaneous/applicationpaths.h"
#include "network-web/webfactory.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/serviceroot.h"

#include <utility>

#include <QCommandLineOption>
#include <QDir>
#include <QUrl>

CommandLineController::CommandLineController(Application* application,
                                             ApplicationPaths* paths,
                                             const QStringList& raw_arguments)
  : m_application(application), m_paths(paths), m_rawArguments(raw_arguments), m_allowMultipleInstances(false) {}

void CommandLineController::parseMyArguments(const QStringList& arguments, QString& custom_user_agent) {
  fillParser(m_parser);
  m_parser.setApplicationDescription(QSL(APP_NAME));
  m_parser.setSingleDashWordOptionMode(QCommandLineParser::SingleDashWordOptionMode::ParseAsLongOptions);
  m_allowMultipleInstances = false;

  if (!m_parser.parse(arguments)) {
    qCriticalNN << LOGSEC_CORE << m_parser.errorText();
  }

  if (!m_parser.value(QSL(CLI_DAT_SHORT)).isEmpty()) {
    const QString data_folder = QDir::toNativeSeparators(m_parser.value(QSL(CLI_DAT_SHORT)));

    qDebugNN << LOGSEC_CORE << "User wants to use custom directory for user data (and disable single instance mode):"
             << QUOTE_W_SPACE_DOT(data_folder);
    m_allowMultipleInstances = m_paths->setCustomDataFolder(data_folder);
  }

  if (m_parser.isSet(QSL(CLI_HELP_SHORT))) {
    m_parser.showHelp();
  }
  else if (m_parser.isSet(QSL(CLI_VER_SHORT))) {
    m_parser.showVersion();
  }

  if (m_parser.isSet(QSL(CLI_SIN_SHORT))) {
    m_allowMultipleInstances = true;
    qDebugNN << LOGSEC_CORE << "Explicitly allowing this instance to run.";
  }

  custom_user_agent = m_parser.value(QSL(CLI_USERAGENT_SHORT));
}

void CommandLineController::parseOtherInstanceArguments(const QString& message) {
  if (message.isEmpty()) {
    qDebugNN << LOGSEC_CORE << "No execution message received from other app instances.";
    return;
  }

  qDebugNN << LOGSEC_CORE << "Received" << QUOTE_W_SPACE(message) << "execution message.";

  QStringList arguments = message.split(QSL(ARGUMENTS_LIST_SEPARATOR), SPLIT_BEHAVIOR::SkipEmptyParts);
  QCommandLineParser parser;

  arguments.prepend(m_application->applicationFilePath());
  parser.addOption(QCommandLineOption({QSL(CLI_QUIT_INSTANCE)}));
  parser.addOption(QCommandLineOption({QSL(CLI_IS_RUNNING)}));
  fillParser(parser);

  if (!parser.parse(arguments)) {
    qCriticalNN << LOGSEC_CORE << parser.errorText();
  }

  if (parser.isSet(QSL(CLI_QUIT_INSTANCE))) {
    m_application->quit();
    return;
  }
  else if (parser.isSet(QSL(CLI_IS_RUNNING))) {
    m_application->showGuiMessage(Notification::Event::GeneralEvent,
                                  {m_application->tr("Already running"),
                                   m_application->tr("Application is already running."),
                                   QSystemTrayIcon::MessageIcon::Information});
    m_application->mainForm()->display();
  }

  const QStringList positional_arguments = parser.positionalArguments();

  for (const QString& argument : positional_arguments) {
    if (argument.trimmed().size() < 3) {
      continue;
    }

    const QString processed_url = m_application->web()->processFeedUriScheme(argument.trimmed());
    if (QUrl::fromUserInput(processed_url).scheme().isEmpty()) {
      continue;
    }

    auto service_root =
      qlinq::from(m_application->feedReader()->feedsModel()->serviceRoots()).firstOrDefault([](ServiceRoot* root) {
        return root->supportsFeedAdding();
      });

    if (service_root.has_value()) {
      service_root.value()->addNewFeed(nullptr, processed_url);
    }
    else {
      m_application
        ->showGuiMessage(Notification::Event::GeneralEvent,
                         {m_application->tr("Cannot add feed"),
                          m_application
                            ->tr("Feed cannot be added because there is no active account which can add feeds."),
                          QSystemTrayIcon::MessageIcon::Warning});
    }
  }
}

bool CommandLineController::isAlreadyRunning() {
  return !m_allowMultipleInstances && m_application->sendMessage((QStringList() << QSL("-%1").arg(QSL(CLI_IS_RUNNING))
                                                                                << Application::arguments().mid(1))
                                                                   .join(QSL(ARGUMENTS_LIST_SEPARATOR)));
}

QCommandLineParser* CommandLineController::parser() {
  return &m_parser;
}

QStringList CommandLineController::rawArguments() const {
  return m_rawArguments;
}

void CommandLineController::fillParser(QCommandLineParser& parser) const {
  QCommandLineOption help({QSL(CLI_HELP_SHORT), QSL(CLI_HELP_LONG)}, QSL("Displays overview of CLI."));
  QCommandLineOption version({QSL(CLI_VER_SHORT), QSL(CLI_VER_LONG)}, QSL("Displays version of the application."));
  QCommandLineOption
    custom_data_folder({QSL(CLI_DAT_SHORT), QSL(CLI_DAT_LONG)},
                       QSL("Use custom folder for user data and disable single instance application mode."),
                       QSL("user-data-folder"));
  QCommandLineOption disable_singleinstance({QSL(CLI_SIN_SHORT), QSL(CLI_SIN_LONG)},
                                            QSL("Allow running of multiple application instances."));
  QCommandLineOption log_to_file({QSL(CLI_LOG_SHORT), QSL(CLI_LOG_LONG)},
                                 QSL("Log application standard/error output to file. When empty string is provided as "
                                     "argument, then the log file will be stored in user data folder."),
                                 QSL("log-file-name"));
  QCommandLineOption debug_output({QSL(CLI_DEBUG_SHORT), QSL(CLI_DEBUG_LONG)}, QSL("Enable \"debug\" CLI output."));
  QCommandLineOption force_text_viewer(QSL(CLI_FORCETEXT_LONG), QSL("Force QTextBrowser-based article viewer."));
  QCommandLineOption forced_style({QSL(CLI_STYLE_SHORT), QSL(CLI_STYLE_LONG)},
                                  QSL("Force some application style."),
                                  QSL("style-name"));
  QCommandLineOption custom_user_agent({QSL(CLI_USERAGENT_SHORT), QSL(CLI_USERAGENT_LONG)},
                                       QSL("User custom User-Agent HTTP header for all network requests. This option "
                                           "takes precedence over User-Agent set via application settings."),
                                       QSL("user-agent"));
  QCommandLineOption custom_threads(QSL(CLI_THREADS),
                                    QSL("Specify number of threads. Note that number cannot be higher than %1.")
                                      .arg(MAX_THREADPOOL_THREADS),
                                    QSL("count"));

  parser.addOptions({help,
                     version,
                     custom_data_folder,
                     disable_singleinstance,
                     debug_output,
                     force_text_viewer,
                     log_to_file,
                     forced_style,
                     custom_user_agent,
                     custom_threads});
  parser.addPositionalArgument(QSL("urls"),
                               QSL("List of URL addresses pointing to individual online feeds which should be added."),
                               QSL("[url-1 ... url-n]"));
}
