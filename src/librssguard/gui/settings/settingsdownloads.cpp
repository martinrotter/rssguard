// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsdownloads.h"

#include "gui/dialogs/filedialog.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "network-web/downloadmanager.h"

SettingsDownloads::SettingsDownloads(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsDownloads) {
  m_ui->setupUi(this);
  connect(m_ui->m_checkOpenManagerWhenDownloadStarts, &QCheckBox::toggled, this, &SettingsDownloads::dirtifySettings);
  connect(m_ui->m_txtDownloadsTargetDirectory, &QLineEdit::textChanged, this, &SettingsDownloads::dirtifySettings);
  connect(m_ui->m_rbDownloadsAskEachFile, &QRadioButton::toggled, this, &SettingsDownloads::dirtifySettings);
  connect(m_ui->m_btnDownloadsTargetDirectory,
          &QPushButton::clicked,
          this,
          &SettingsDownloads::selectDownloadsDirectory);
}

SettingsDownloads::~SettingsDownloads() {
  delete m_ui;
}

QIcon SettingsDownloads::icon() const {
  return qApp->icons()->fromTheme(QSL("browser-downloads"), QSL("download"));
}

void SettingsDownloads::selectDownloadsDirectory() {
  const QString target_directory = FileDialog::existingDirectory(this,
                                                                 tr("Select downloads target directory"),
                                                                 m_ui->m_txtDownloadsTargetDirectory->text());

  if (!target_directory.isEmpty()) {
    m_ui->m_txtDownloadsTargetDirectory->setText(QDir::toNativeSeparators(target_directory));
  }
}

void SettingsDownloads::loadSettings() {
  onBeginLoadSettings();
  m_ui->m_checkOpenManagerWhenDownloadStarts
    ->setChecked(settings()->value(GROUP(Downloads), SETTING(Downloads::ShowDownloadsWhenNewDownloadStarts)).toBool());
  m_ui->m_txtDownloadsTargetDirectory->setText(QDir::toNativeSeparators(settings()
                                                                          ->value(GROUP(Downloads),
                                                                                  SETTING(Downloads::TargetDirectory))
                                                                          .toString()));
  m_ui->m_rbDownloadsAskEachFile
    ->setChecked(settings()->value(GROUP(Downloads), SETTING(Downloads::AlwaysPromptForFilename)).toBool());
  onEndLoadSettings();
}

void SettingsDownloads::saveSettings() {
  onBeginSaveSettings();
  settings()->setValue(GROUP(Downloads),
                       Downloads::ShowDownloadsWhenNewDownloadStarts,
                       m_ui->m_checkOpenManagerWhenDownloadStarts->isChecked());
  settings()->setValue(GROUP(Downloads), Downloads::TargetDirectory, m_ui->m_txtDownloadsTargetDirectory->text());
  settings()->setValue(GROUP(Downloads),
                       Downloads::AlwaysPromptForFilename,
                       m_ui->m_rbDownloadsAskEachFile->isChecked());
  qApp->downloadManager()->setDownloadDirectory(m_ui->m_txtDownloadsTargetDirectory->text());
  onEndSaveSettings();
}
