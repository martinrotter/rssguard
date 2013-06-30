#include <QMessageBox>
#include <QProcess>

#include "gui/formsettings.h"
#include "gui/themefactory.h"
#include "gui/systemtrayicon.h"
#include "gui/formmain.h"
#include "core/settings.h"
#include "core/defs.h"
#include "core/localization.h"
#include "core/systemfactory.h"
#include "core/dynamicshortcuts.h"


FormSettings::FormSettings(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormSettings) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(ThemeFactory::fromTheme("preferences-system"));

  // Setup behavior.
  m_ui->m_treeLanguages->setColumnCount(5);
  m_ui->m_treeLanguages->setHeaderHidden(false);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->setHeaderLabels(QStringList()
                                         << tr("Language")
                                         << tr("Code")
                                         << tr("Version")
                                         << tr("Author")
                                         << tr("Email"));

  // Establish needed connections.
  connect(this, &FormSettings::accepted, this, &FormSettings::saveSettings);

  // Load all settings.
  loadGeneral();
  loadShortcuts();
  loadInterface();
  loadLanguage();
}

FormSettings::~FormSettings() {
  delete m_ui;
}

void FormSettings::saveSettings() {
  // Save all settings.
  saveGeneral();
  saveShortcuts();
  saveInterface();
  saveLanguage();
}

void FormSettings::loadLanguage() {
  QList<Language> languages = Localization::getInstalledLanguages();

  foreach (Language lang, languages) {
    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->m_treeLanguages);
    item->setText(0, lang.m_name);
    item->setText(1, lang.m_code);
    item->setText(2, lang.m_version);
    item->setText(3, lang.m_author);
    item->setText(4, lang.m_email);
    item->setIcon(0, QIcon(APP_FLAGS_PATH + "/" + lang.m_code + ".png"));
  }

  QList<QTreeWidgetItem*> matching_items = m_ui->m_treeLanguages->findItems(Settings::getInstance()->value(APP_CFG_GEN,
                                                                                                           "language",
                                                                                                           "en").toString(),
                                                                            Qt::MatchExactly,
                                                                            1);
  if (!matching_items.isEmpty()) {
    m_ui->m_treeLanguages->setCurrentItem(matching_items[0]);
  }
}

void FormSettings::saveLanguage() {
  if (m_ui->m_treeLanguages->currentItem() == nullptr) {
    qDebug("No localizations loaded in settings dialog, so no saving for them.");
    return;
  }

  QString actual_lang = Settings::getInstance()->value(APP_CFG_GEN,
                                                       "language",
                                                       "en").toString();
  QString new_lang = m_ui->m_treeLanguages->currentItem()->text(1);

  if (new_lang != actual_lang) {
    Settings::getInstance()->setValue(APP_CFG_GEN, "language", new_lang);

    QMessageBox msg_question(this);
    msg_question.setText(tr("Language of Qonverter was changed. Note that changes will take effect on next Qonverter start."));
    msg_question.setInformativeText(tr("Do you want to restart now?"));
    msg_question.setWindowTitle(tr("Language changed"));
    msg_question.setIcon(QMessageBox::Question);
    msg_question.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg_question.setDefaultButton(QMessageBox::Yes);

    if (msg_question.exec() == QMessageBox::Yes) {
      if (!QProcess::startDetached(qApp->applicationFilePath())) {
        QMessageBox::warning(this,
                             tr("Problem with RSS Guard restart"),
                             tr("Qonverter couldn't be restarted, please restart it manually for changes to take effect."));
      }
      else {
        qApp->quit();
      }
    }
  }
}

void FormSettings::loadShortcuts() {
  m_ui->m_shortcuts->populate(FormMain::getInstance()->getActions());
}

void FormSettings::saveShortcuts() {
  // Update the actual shortcuts of some actions.
  m_ui->m_shortcuts->updateShortcuts();

  // Save new shortcuts to the settings.
  DynamicShortcuts::save(FormMain::getInstance()->getActions());
}

void FormSettings::loadGeneral() {
  // Load auto-start status.
  SystemFactory::AutoStartStatus autostart_status = SystemFactory::getAutoStartStatus();
  switch (autostart_status) {
    case SystemFactory::Enabled:
      m_ui->m_checkAutostart->setChecked(true);
      break;
    case SystemFactory::Disabled:
      m_ui->m_checkAutostart->setChecked(false);
      break;
    default:
      m_ui->m_checkAutostart->setEnabled(false);
      m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text() +
                                      tr(" (not supported on this platform)"));
      break;
  }
}

void FormSettings::saveGeneral() {
  // If auto-start feature is available and user wants
  // to turn it on, then turn it on.
  if (SystemFactory::getAutoStartStatus() != SystemFactory::Unavailable) {
    if (m_ui->m_checkAutostart->isChecked()) {
      SystemFactory::setAutoStartStatus(SystemFactory::Enabled);
    }
    else {
      SystemFactory::setAutoStartStatus(SystemFactory::Disabled);
    }
  }
}

void FormSettings::loadInterface() {
  // Load settings of tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    m_ui->m_radioTrayOff->setChecked(!Settings::getInstance()->value(APP_CFG_GUI,
                                                                     "use_tray_icon",
                                                                     true).toBool());
    m_ui->m_cmbTrayClose->setCurrentIndex(Settings::getInstance()->value(APP_CFG_GUI,
                                                                         "close_win_action",
                                                                         0).toInt());
    m_ui->m_checkHidden->setChecked(Settings::getInstance()->value(APP_CFG_GUI,
                                                                   "start_hidden",
                                                                   false).toBool());
  }
  // Tray icon is not supported on this machine.
  else {
    m_ui->m_radioTrayOff->setText(tr("disable (Tray icon is not available.)"));
    m_ui->m_radioTrayOff->setChecked(true);
    m_ui->m_grpTray->setDisabled(true);
  }

  // Load settings of icon theme.
  QString current_theme = ThemeFactory::getCurrentIconTheme();
#if defined(Q_OS_LINUX)
  QString system_theme = ThemeFactory::getSystemIconTheme();
#endif

  foreach (QString icon_theme_name, ThemeFactory::getInstalledIconThemes()) {
#if defined(Q_OS_LINUX)
    if (icon_theme_name == system_theme) {
      m_ui->m_cmbIconTheme->addItem(tr("system icon theme (default)"),
                                    icon_theme_name);
    }
    else {
#endif
      m_ui->m_cmbIconTheme->addItem(icon_theme_name,
                                    icon_theme_name);
#if defined(Q_OS_LINUX)
    }
    if (current_theme == system_theme) {
      // Because system icon theme lies at the index 0.
      // See ThemeFactory::getInstalledIconThemes() for more info.
      m_ui->m_cmbIconTheme->setCurrentIndex(0);
    }
    else {
#endif
      m_ui->m_cmbIconTheme->setCurrentText(current_theme);
#if defined(Q_OS_LINUX)
    }
#endif
  }
}

void FormSettings::saveInterface() {
  // Save tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    Settings::getInstance()->setValue(APP_CFG_GUI, "use_tray_icon",
                                      m_ui->m_radioTrayOn->isChecked());
    Settings::getInstance()->setValue(APP_CFG_GUI, "close_win_action",
                                      m_ui->m_cmbTrayClose->currentIndex());
    Settings::getInstance()->setValue(APP_CFG_GUI, "start_hidden",
                                      m_ui->m_checkHidden->isChecked());
    if (Settings::getInstance()->value(APP_CFG_GUI, "use_tray_icon", true).toBool()) {
      SystemTrayIcon::getInstance()->show();
    }
    else {
      FormMain::getInstance()->display();
      SystemTrayIcon::deleteInstance();
    }
  }

  // Save selected icon theme.
  ThemeFactory::setCurrentIconTheme(m_ui->m_cmbIconTheme->itemData(m_ui->m_cmbIconTheme->currentIndex()).toString());
}
