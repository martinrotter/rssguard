// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsnodejs.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/filedialog.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/nodejs.h"
#include "network-web/webfactory.h"

#include <QDir>

SettingsNodejs::SettingsNodejs(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent) {
  m_ui.setupUi(this);

  m_ui.m_helpInfo->setHelpText(tr("What is Node.js?"),
                               tr("Node.js is asynchronous event-driven JavaScript runtime, designed to build "
                                  "scalable network applications.\n\n"
                                  "%1 integrates Node.js to bring some modern features like Adblock.\n\n"
                                  "Note that usually all required Node.js tools should be available via your \"PATH\" "
                                  "environment variable, so you do not have to specify full paths.\n\n"
                                  "Also, relaunch \"Settings\" dialog after you install Node.js.")
                                 .arg(APP_NAME),
                               false);

  m_ui.m_helpPackages
    ->setHelpText(tr("%1 automatically installs some Node.js packages so that you do not have to. %1 does not "
                     "use global package folder because that requires administrator rights, therefore by default "
                     "it uses subfolder placed in your \"user data\" folder.")
                    .arg(APP_NAME),
                  false);

  connect(m_ui.m_btnDownloadNodejs, &QPushButton::clicked, this, [this]() {
    qApp->web()->openUrlInExternalBrowser(QSL("https://nodejs.org/en/download/"));
  });

  connect(m_ui.m_tbNodeExecutable->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsNodejs::testNodejs);
  connect(m_ui.m_tbNpmExecutable->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsNodejs::testNpm);
  connect(m_ui.m_tbPackageFolder->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsNodejs::testPackageFolder);

  connect(m_ui.m_tbNodeExecutable->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsNodejs::dirtifySettings);
  connect(m_ui.m_tbNpmExecutable->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsNodejs::dirtifySettings);
  connect(m_ui.m_tbPackageFolder->lineEdit(), &BaseLineEdit::textChanged, this, &SettingsNodejs::dirtifySettings);

  connect(m_ui.m_btnPackageFolder, &QPushButton::clicked, this, [this]() {
    changeFileFolder(m_ui.m_tbPackageFolder, true);
  });
  connect(m_ui.m_btnNodeExecutable, &QPushButton::clicked, this, [this]() {
    changeFileFolder(m_ui.m_tbNodeExecutable, false, QSL("Node.js (node*)"));
  });
  connect(m_ui.m_btnNpmExecutable, &QPushButton::clicked, this, [this]() {
    changeFileFolder(m_ui.m_tbNpmExecutable, false, QSL("NPM (npm*)"));
  });
}

QIcon SettingsNodejs::icon() const {
  return qApp->icons()->fromTheme(QSL("node-join"), QSL("node"));
}

void SettingsNodejs::changeFileFolder(LineEditWithStatus* tb, bool directory_select, const QString& file_filter) {
  QString file_dir;
  QString current = qApp->replaceUserDataFolderPlaceholder(tb->lineEdit()->text());

  if (directory_select) {
    file_dir = FileDialog::existingDirectory(this, {}, current, GENERAL_REMEMBERED_PATH);
  }
  else {
    file_dir = FileDialog::openFileName(this, {}, current, file_filter, {}, GENERAL_REMEMBERED_PATH);
  }

  if (!file_dir.isEmpty()) {
    tb->lineEdit()->setText(QDir::toNativeSeparators(file_dir));
  }
}

QString SettingsNodejs::title() const {
  return QSL("Node.js");
}

void SettingsNodejs::loadSettings() {
  onBeginLoadSettings();

  m_ui.m_tbNodeExecutable->lineEdit()->setText(qApp->nodejs()->nodeJsExecutable());
  m_ui.m_tbNpmExecutable->lineEdit()->setText(qApp->nodejs()->npmExecutable());
  m_ui.m_tbPackageFolder->lineEdit()->setText(qApp->nodejs()->packageFolder());

  onEndLoadSettings();
}

void SettingsNodejs::saveSettings() {
  onBeginSaveSettings();

  qApp->nodejs()->setNodeJsExecutable(m_ui.m_tbNodeExecutable->lineEdit()->text());
  qApp->nodejs()->setNpmExecutable(m_ui.m_tbNpmExecutable->lineEdit()->text());
  qApp->nodejs()->setPackageFolder(m_ui.m_tbPackageFolder->lineEdit()->text());

  onEndSaveSettings();
}

void SettingsNodejs::testNodejs() {
  try {
    QString node_version = qApp->nodejs()->nodeJsVersion(m_ui.m_tbNodeExecutable->lineEdit()->text());

    m_ui.m_tbNodeExecutable->setStatus(WidgetWithStatus::StatusType::Ok,
                                       tr("Node.js has version %1.").arg(node_version));
  }
  catch (const ApplicationException& ex) {
    m_ui.m_tbNodeExecutable->setStatus(WidgetWithStatus::StatusType::Error, QSL("Node.js: %1.").arg(ex.message()));
  }
}

void SettingsNodejs::testNpm() {
  try {
    QString npm_version = qApp->nodejs()->npmVersion(m_ui.m_tbNpmExecutable->lineEdit()->text());

    m_ui.m_tbNpmExecutable->setStatus(WidgetWithStatus::StatusType::Ok, tr("NPM has version %1.").arg(npm_version));
  }
  catch (const ApplicationException& ex) {
    m_ui.m_tbNpmExecutable->setStatus(WidgetWithStatus::StatusType::Error, QSL("NPM: %1.").arg(ex.message()));
  }
}

void SettingsNodejs::testPackageFolder() {
  QString folder = qApp->replaceUserDataFolderPlaceholder(m_ui.m_tbPackageFolder->lineEdit()->text());

  const auto fi = QFileInfo(folder);
  const auto is_file = fi.isFile() && fi.exists();
  QString desc;
  WidgetWithStatus::StatusType stat;

  if (is_file) {
    stat = WidgetWithStatus::StatusType::Error;
    desc = tr("You cannot choose file, you have to choose FOLDER.");
  }
  else if (QDir().exists(folder)) {
    stat = WidgetWithStatus::StatusType::Ok;
    desc = tr("Package folder is OK.");
  }
  else {
    stat = WidgetWithStatus::StatusType::Ok;
    desc = tr("Package folder will be created!");
  }

  m_ui.m_tbPackageFolder->setStatus(stat, desc);
}
