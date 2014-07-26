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

#include "gui/formsettings.h"

#include "definitions/definitions.h"
#include "core/feeddownloader.h"
#include "core/feedsmodel.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/localization.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/skinfactory.h"
#include "network-web/webfactory.h"
#include "network-web/webbrowsernetworkaccessmanager.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webbrowser.h"
#include "gui/systemtrayicon.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/feedstoolbar.h"
#include "gui/formmain.h"
#include "gui/messagebox.h"
#include "gui/basetoolbar.h"
#include "gui/messagestoolbar.h"
#include "dynamic-shortcuts/dynamicshortcuts.h"
#include "application.h"

#include <QProcess>
#include <QNetworkProxy>
#include <QColorDialog>
#include <QFileDialog>


FormSettings::FormSettings(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormSettings) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconFactory::instance()->fromTheme("application-settings"));

#if !defined(Q_OS_WIN)
  MessageBox::iconify(m_ui->m_buttonBox);
#endif

  // Setup behavior.
  m_ui->m_listSettings->setCurrentRow(0);
  m_ui->m_treeLanguages->setColumnCount(5);
  m_ui->m_treeLanguages->setHeaderHidden(false);
  m_ui->m_treeLanguages->setHeaderLabels(QStringList()
                                         << /*: Language column of language list. */ tr("Language")
                                         << /*: Lang. code column of language list. */ tr("Code")
                                         << tr("Version")
                                         << tr("Author")
                                         << tr("Email"));

  m_ui->m_treeSkins->setColumnCount(4);
  m_ui->m_treeSkins->setHeaderHidden(false);
  m_ui->m_treeSkins->setHeaderLabels(QStringList()
                                     << /*: Skin list name column. */ tr("Name")
                                     << /*: Version column of skin list. */ tr("Version")
                                     << tr("Author")
                                     << tr("Email"));

#if QT_VERSION >= 0x050000
  // Setup languages.
  m_ui->m_treeLanguages->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

  // Setup skins.
  m_ui->m_treeSkins->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
#else
  // Setup languages.
  m_ui->m_treeLanguages->header()->setResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setResizeMode(3, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setResizeMode(4, QHeaderView::ResizeToContents);

  // Setup skins.
  m_ui->m_treeSkins->header()->setResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setResizeMode(3, QHeaderView::ResizeToContents);
#endif

  // Establish needed connections.
  connect(m_ui->m_buttonBox, SIGNAL(accepted()),
          this, SLOT(saveSettings()));
  connect(m_ui->m_cmbProxyType, SIGNAL(currentIndexChanged(int)),
          this, SLOT(onProxyTypeChanged(int)));
  connect(m_ui->m_checkShowPassword, SIGNAL(stateChanged(int)),
          this, SLOT(displayProxyPassword(int)));
  connect(m_ui->m_btnWebBrowserColorSample, SIGNAL(clicked()),
          this, SLOT(changeBrowserProgressColor()));
  connect(m_ui->m_treeSkins, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(onSkinSelected(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(m_ui->m_cmbExternalBrowserPreset, SIGNAL(currentIndexChanged(int)),
          this, SLOT(changeDefaultBrowserArguments(int)));
  connect(m_ui->m_btnExternalBrowserExecutable, SIGNAL(clicked()),
          this, SLOT(selectBrowserExecutable()));
  connect(m_ui->m_txtMysqlUsername->lineEdit(), SIGNAL(textChanged(QString)),
          this, SLOT(onMysqlUsernameChanged(QString)));
  connect(m_ui->m_txtMysqlHostname->lineEdit(), SIGNAL(textChanged(QString)),
          this, SLOT(onMysqlHostnameChanged(QString)));
  connect(m_ui->m_txtMysqlPassword->lineEdit(), SIGNAL(textChanged(QString)),
          this, SLOT(onMysqlPasswordChanged(QString)));
  connect(m_ui->m_btnMysqlTestSetup, SIGNAL(clicked()),
          this, SLOT(mysqlTestConnection()));
  connect(m_ui->m_spinMysqlPort, SIGNAL(editingFinished()),
          this, SLOT(onMysqlDataStorageEdited()));
  connect(m_ui->m_txtMysqlHostname->lineEdit(), SIGNAL(textEdited(QString)),
          this, SLOT(onMysqlDataStorageEdited()));
  connect(m_ui->m_txtMysqlPassword->lineEdit(), SIGNAL(textEdited(QString)),
          this, SLOT(onMysqlDataStorageEdited()));
  connect(m_ui->m_txtMysqlUsername->lineEdit(), SIGNAL(textEdited(QString)),
          this, SLOT(onMysqlDataStorageEdited()));
  connect(m_ui->m_cmbSelectToolBar, SIGNAL(currentIndexChanged(int)),
          m_ui->m_stackedToolbars, SLOT(setCurrentIndex(int)));

  // Load all settings.
  loadGeneral();
  loadDataStorage();
  loadShortcuts();
  loadInterface();
  loadProxy();
  loadBrowser();
  loadLanguage();
  loadFeedsMessages();
}

FormSettings::~FormSettings() {
  qDebug("Destroying FormSettings distance.");
  delete m_ui;
}

void FormSettings::changeDefaultBrowserArguments(int index) {
  if (index != 0) {
    m_ui->m_txtExternalBrowserArguments->setText(m_ui->m_cmbExternalBrowserPreset->itemData(index).toString());
  }
}

void FormSettings::onSkinSelected(QTreeWidgetItem *current,
                                  QTreeWidgetItem *previous) {
  Q_UNUSED(previous)

  if (current != NULL) {
    Skin skin = current->data(0, Qt::UserRole).value<Skin>();
    m_ui->m_lblSelectedContents->setText(skin.m_visibleName);
  }
}

void FormSettings::changeBrowserProgressColor() {
  QPointer<QColorDialog> color_dialog = new QColorDialog(m_initialSettings.m_webBrowserProgress,
                                                         this);
  color_dialog.data()->setWindowTitle(tr("Select color for web browser progress bar"));
  color_dialog.data()->setOption(QColorDialog::ShowAlphaChannel, false);

  if (color_dialog.data()->exec() == QDialog::Accepted) {
    m_initialSettings.m_webBrowserProgress = color_dialog.data()->selectedColor();
    loadWebBrowserColor(m_initialSettings.m_webBrowserProgress);
  }

  delete color_dialog.data();
}

void FormSettings::selectBrowserExecutable() {
  QString executable_file = QFileDialog::getOpenFileName(this,
                                                         tr("Select web browser executable"),
                                                         QDir::homePath(),
                                                         //: File filter for external browser selection dialog.
                                                         tr("Executables (*.*)"));

  if (!executable_file.isEmpty()) {
    m_ui->m_txtExternalBrowserExecutable->setText(executable_file);
  }
}

void FormSettings::loadFeedsMessages() {
  Settings *settings = qApp->settings();

  m_ui->m_checkKeppMessagesInTheMiddle->setChecked(settings->value(APP_CFG_MESSAGES, "keep_cursor_center", false).toBool());
  m_ui->m_checkRemoveReadMessagesOnExit->setChecked(settings->value(APP_CFG_MESSAGES, "clear_read_on_exit", false).toBool());
  m_ui->m_checkAutoUpdate->setChecked(settings->value(APP_CFG_FEEDS, "auto_update_enabled", false).toBool());
  m_ui->m_spinAutoUpdateInterval->setValue(settings->value(APP_CFG_FEEDS, "auto_update_interval", DEFAULT_AUTO_UPDATE_INTERVAL).toInt());
  m_ui->m_spinFeedUpdateTimeout->setValue(settings->value(APP_CFG_FEEDS, "feed_update_timeout", DOWNLOAD_TIMEOUT).toInt());
  m_ui->m_checkUpdateAllFeedsOnStartup->setChecked(settings->value(APP_CFG_FEEDS, "feeds_update_on_startup", false).toBool());
  m_ui->m_cmbCountsFeedList->addItems(QStringList() << "(%unread)" << "[%unread]" << "%unread/%all" << "%unread-%all" << "[%unread|%all]");
  m_ui->m_cmbCountsFeedList->setEditText(settings->value(APP_CFG_FEEDS, "count_format", "(%unread)").toString());
}

void FormSettings::saveFeedsMessages() {
  Settings *settings = qApp->settings();

  settings->setValue(APP_CFG_MESSAGES, "keep_cursor_center", m_ui->m_checkKeppMessagesInTheMiddle->isChecked());
  settings->setValue(APP_CFG_MESSAGES, "clear_read_on_exit", m_ui->m_checkRemoveReadMessagesOnExit->isChecked());
  settings->setValue(APP_CFG_FEEDS, "auto_update_enabled", m_ui->m_checkAutoUpdate->isChecked());
  settings->setValue(APP_CFG_FEEDS, "auto_update_interval", m_ui->m_spinAutoUpdateInterval->value());
  settings->setValue(APP_CFG_FEEDS, "feed_update_timeout", m_ui->m_spinFeedUpdateTimeout->value());
  settings->setValue(APP_CFG_FEEDS, "feeds_update_on_startup", m_ui->m_checkUpdateAllFeedsOnStartup->isChecked());
  settings->setValue(APP_CFG_FEEDS, "count_format", m_ui->m_cmbCountsFeedList->currentText());

  FormMain::instance()->tabWidget()->feedMessageViewer()->feedsView()->updateAutoUpdateStatus();
  FormMain::instance()->tabWidget()->feedMessageViewer()->feedsView()->sourceModel()->reloadWholeLayout();
}

void FormSettings::displayProxyPassword(int state) {
  if (state == Qt::Checked) {
    m_ui->m_txtProxyPassword->setEchoMode(QLineEdit::Normal);
  }
  else {
    m_ui->m_txtProxyPassword->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  }
}

bool FormSettings::doSaveCheck() {
  bool everything_ok = true;
  QStringList resulting_information;

  // Setup indication of settings consistence.
  if (!m_ui->m_shortcuts->areShortcutsUnique()) {
    everything_ok = false;
    resulting_information.append(tr("some keyboard shortcuts are not unique"));
  }

  // User selected custom external browser but did not set its
  // properties.
  if (m_ui->m_grpCustomExternalBrowser->isChecked() &&
      (m_ui->m_txtExternalBrowserExecutable->text().simplified().isEmpty() ||
       !m_ui->m_txtExternalBrowserArguments->text().simplified().contains("%1"))) {
    everything_ok = false;
    resulting_information.append(tr("custom external browser is not set correctly"));
  }

  if (!everything_ok) {
    resulting_information.replaceInStrings(QRegExp("^"),
                                           QString::fromUtf8(" • "));

    MessageBox::show(this,
                     QMessageBox::Critical,
                     tr("Cannot save settings"),
                     tr("Some critical settings are not set. You must fix these settings in order confirm new settings."),
                     QString(),
                     tr("List of errors:\n%1.").arg(resulting_information.join(",\n")));
  }

  return everything_ok;
}

void FormSettings::loadWebBrowserColor(const QColor &color) {
  m_ui->m_btnWebBrowserColorSample->setStyleSheet(QString("QToolButton { background-color: rgba(%1, %2, %3, %4); }").arg(QString::number(color.red()),
                                                                                                                         QString::number(color.green()),
                                                                                                                         QString::number(color.blue()),
                                                                                                                         QString::number(color.alpha())));
}

void FormSettings::promptForRestart() {
  if (m_changedDataTexts.count() > 0) {
    QStringList changed_data_texts = m_changedDataTexts;

    changed_data_texts.replaceInStrings(QRegExp("^"),
                                        QString::fromUtf8(" • "));

    int question_result = MessageBox::show(this,
                                           QMessageBox::Question,
                                           tr("Critical settings were changed"),
                                           tr("Some critical settings were changed and will be applied after the application gets restarted."),
                                           tr("Do you want to restart now?"),
                                           tr("List of changes:\n%1.").arg(changed_data_texts.join(",\n")),
                                           QMessageBox::Yes | QMessageBox::No,
                                           QMessageBox::Yes);

    if (question_result == QMessageBox::Yes) {
      if (!QProcess::startDetached(qApp->applicationFilePath())) {
        if (SystemTrayIcon::isSystemTrayActivated()) {
          SystemTrayIcon::instance()->showMessage(tr("Problem with application restart"),
                                                  tr("Application couldn't be restarted. "
                                                     "Please, restart it manually for changes to take effect."),
                                                  QSystemTrayIcon::Warning);
        }
        else {
          MessageBox::show(this,
                           QMessageBox::Warning,
                           tr("Problem with application restart"),
                           tr("Application couldn't be restarted. "
                              "Please, restart it manually for changes to take effect."));
        }
      }
      else {
        qApp->quit();
      }
    }
  }
}

void FormSettings::saveSettings() {
  // Make sure everything is saveable.
  if (!doSaveCheck()) {
    return;
  }

  // Save all settings.
  saveGeneral();
  saveDataStorage();
  saveShortcuts();
  saveInterface();
  saveProxy();
  saveBrowser();
  saveLanguage();
  saveFeedsMessages();

  qApp->settings()->checkSettings();
  promptForRestart();

  accept();
}

void FormSettings::onProxyTypeChanged(int index) {
  QNetworkProxy::ProxyType selected_type = static_cast<QNetworkProxy::ProxyType>(m_ui->m_cmbProxyType->itemData(index).toInt());
  bool is_proxy_selected = selected_type != QNetworkProxy::NoProxy && selected_type != QNetworkProxy::DefaultProxy;

  m_ui->m_txtProxyHost->setEnabled(is_proxy_selected);
  m_ui->m_txtProxyPassword->setEnabled(is_proxy_selected);
  m_ui->m_txtProxyUsername->setEnabled(is_proxy_selected);
  m_ui->m_spinProxyPort->setEnabled(is_proxy_selected);
  m_ui->m_checkShowPassword->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyHost->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyInfo->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyPassword->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyPort->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyUsername->setEnabled(is_proxy_selected);
}

void FormSettings::loadBrowser() {
  Settings *settings = qApp->settings();

  // Load settings of web browser GUI.
  m_initialSettings.m_webBrowserProgress = QColor(settings->value(APP_CFG_BROWSER,
                                                                  "browser_progress_color",
                                                                  QColor(155, 250, 80)).toString());
  m_ui->m_checkBrowserProgressColor->setChecked(settings->value(APP_CFG_BROWSER,
                                                                "browser_colored_progress_enabled",
                                                                true).toBool());
  m_ui->m_checkMouseGestures->setChecked(settings->value(APP_CFG_BROWSER,
                                                         "gestures_enabled",
                                                         true).toBool());
  m_ui->m_checkQueueTabs->setChecked(settings->value(APP_CFG_BROWSER,
                                                     "queue_tabs",
                                                     true).toBool());
  m_ui->m_btnWebBrowserColorSample->setMaximumHeight(m_ui->m_checkBrowserProgressColor->sizeHint().height());
  loadWebBrowserColor(m_initialSettings.m_webBrowserProgress);

  m_ui->m_cmbExternalBrowserPreset->addItem(tr("Opera 12 or older"), "-nosession %1");
  m_ui->m_txtExternalBrowserExecutable->setText(settings->value(APP_CFG_BROWSER,
                                                                "external_browser_executable").toString());
  m_ui->m_txtExternalBrowserArguments->setText(settings->value(APP_CFG_BROWSER,
                                                               "external_browser_arguments",
                                                               "%1").toString());
  m_ui->m_grpCustomExternalBrowser->setChecked(settings->value(APP_CFG_BROWSER,
                                                               "custom_external_browser",
                                                               false).toBool());
  m_ui->m_checkAutoLoadImages->setChecked(WebFactory::instance()->autoloadImages());
  m_ui->m_checkEnableJavascript->setChecked(WebFactory::instance()->javascriptEnabled());
  m_ui->m_checkEnablePlugins->setChecked(WebFactory::instance()->pluginsEnabled());
}

void FormSettings::saveBrowser() {
  Settings *settings = qApp->settings();

  // Save settings of GUI of web browser.
  settings->setValue(APP_CFG_BROWSER,
                     "custom_external_browser",
                     m_ui->m_grpCustomExternalBrowser->isChecked());
  settings->setValue(APP_CFG_BROWSER,
                     "browser_progress_color",
                     m_initialSettings.m_webBrowserProgress.name());
  settings->setValue(APP_CFG_BROWSER,
                     "browser_colored_progress_enabled",
                     m_ui->m_checkBrowserProgressColor->isChecked());
  settings->setValue(APP_CFG_BROWSER,
                     "gestures_enabled",
                     m_ui->m_checkMouseGestures->isChecked());
  settings->setValue(APP_CFG_BROWSER,
                     "queue_tabs",
                     m_ui->m_checkQueueTabs->isChecked());

  WebFactory::instance()->switchImages(m_ui->m_checkAutoLoadImages->isChecked());
  WebFactory::instance()->switchJavascript(m_ui->m_checkEnableJavascript->isChecked());
  WebFactory::instance()->switchPlugins(m_ui->m_checkEnablePlugins->isChecked());

  settings->setValue(APP_CFG_BROWSER,
                     "external_browser_executable",
                     m_ui->m_txtExternalBrowserExecutable->text());
  settings->setValue(APP_CFG_BROWSER,
                     "external_browser_arguments",
                     m_ui->m_txtExternalBrowserArguments->text());
}

void FormSettings::loadProxy() {
  m_ui->m_cmbProxyType->addItem(tr("No proxy"), QNetworkProxy::NoProxy);
  m_ui->m_cmbProxyType->addItem(tr("System proxy"), QNetworkProxy::DefaultProxy);
  m_ui->m_cmbProxyType->addItem(tr("Socks5"), QNetworkProxy::Socks5Proxy);
  m_ui->m_cmbProxyType->addItem(tr("Http"), QNetworkProxy::HttpProxy);

  // Load the settings.
  Settings *settings = qApp->settings();
  QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(settings->value(APP_CFG_PROXY,
                                                                                                       "proxy_type",
                                                                                                       QNetworkProxy::NoProxy).toInt());

  m_ui->m_cmbProxyType->setCurrentIndex(m_ui->m_cmbProxyType->findData(selected_proxy_type));
  m_ui->m_txtProxyHost->setText(settings->value(APP_CFG_PROXY,
                                                "host").toString());
  m_ui->m_txtProxyUsername->setText(settings->value(APP_CFG_PROXY,
                                                    "username").toString());
  m_ui->m_txtProxyPassword->setText(settings->value(APP_CFG_PROXY,
                                                    "password").toString());
  m_ui->m_spinProxyPort->setValue(settings->value(APP_CFG_PROXY,
                                                  "port", 80).toInt());
}

void FormSettings::saveProxy() {
  Settings *settings = qApp->settings();

  settings->setValue(APP_CFG_PROXY, "proxy_type",
                     m_ui->m_cmbProxyType->itemData(m_ui->m_cmbProxyType->currentIndex()));
  settings->setValue(APP_CFG_PROXY, "host",
                     m_ui->m_txtProxyHost->text());
  settings->setValue(APP_CFG_PROXY, "username",
                     m_ui->m_txtProxyUsername->text());
  settings->setValue(APP_CFG_PROXY, "password",
                     m_ui->m_txtProxyPassword->text());
  settings->setValue(APP_CFG_PROXY, "port",
                     m_ui->m_spinProxyPort->value());

  // Reload settings for all network access managers.
  WebBrowserNetworkAccessManager::instance()->loadSettings();
}

void FormSettings::loadLanguage() {
  foreach (const Language &language, Localization::instance()->installedLanguages()) {
    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->m_treeLanguages);
    item->setText(0, language.m_name);
    item->setText(1, language.m_code);
    item->setText(2, language.m_version);
    item->setText(3, language.m_author);
    item->setText(4, language.m_email);
    item->setIcon(0, IconFactory::instance()->fromTheme(language.m_code));
  }

  QList<QTreeWidgetItem*> matching_items = m_ui->m_treeLanguages->findItems(Localization::instance()->loadedLanguage(),
                                                                            Qt::MatchContains,
                                                                            1);
  if (!matching_items.isEmpty()) {
    m_ui->m_treeLanguages->setCurrentItem(matching_items[0]);
  }
}

void FormSettings::saveLanguage() {
  if (m_ui->m_treeLanguages->currentItem() == NULL) {
    qDebug("No localizations loaded in settings dialog, so no saving for them.");
    return;
  }

  Settings *settings = qApp->settings();
  QString actual_lang = Localization::instance()->loadedLanguage();
  QString new_lang = m_ui->m_treeLanguages->currentItem()->text(1);

  // Save prompt for restart if language has changed.
  if (new_lang != actual_lang) {
    m_changedDataTexts.append(tr("language changed"));
    settings->setValue(APP_CFG_GEN, "language", new_lang);
  }
}

void FormSettings::loadShortcuts() {
  m_ui->m_shortcuts->populate(FormMain::instance()->allActions().values());
}

void FormSettings::saveShortcuts() {
  // Update the actual shortcuts of some actions.
  m_ui->m_shortcuts->updateShortcuts();

  // Save new shortcuts to the settings.
  DynamicShortcuts::save(FormMain::instance()->allActions().values());
}

void FormSettings::loadDataStorage() {
  onMysqlHostnameChanged(QString());
  onMysqlUsernameChanged(QString());
  onMysqlPasswordChanged(QString());

  m_ui->m_lblMysqlTestResult->setStatus(WidgetWithStatus::Information,
                                        tr("No connection test triggered so far."),
                                        tr("You did not executed any connection test yet."));

  // Load SQLite.
  m_ui->m_cmbDatabaseDriver->addItem(
        tr("SQLite (embedded database)"), APP_DB_SQLITE_DRIVER);

  // Load in-memory database status.
  Settings *settings = qApp->settings();

  m_ui->m_checkSqliteUseInMemoryDatabase->setChecked(settings->value(APP_CFG_DB, "use_in_memory_db", false).toBool());

  if (QSqlDatabase::isDriverAvailable(APP_DB_MYSQL_DRIVER)) {
    // Load MySQL.
    m_ui->m_cmbDatabaseDriver->addItem(tr("MySQL/MariaDB (dedicated database)"), APP_DB_MYSQL_DRIVER);

    // Setup placeholders.
    m_ui->m_txtMysqlHostname->lineEdit()->setPlaceholderText(tr("Hostname of your MySQL server"));
    m_ui->m_txtMysqlUsername->lineEdit()->setPlaceholderText(tr("Username to login with"));
    m_ui->m_txtMysqlPassword->lineEdit()->setPlaceholderText(tr("Password for your username"));

    m_ui->m_txtMysqlHostname->lineEdit()->setText(settings->value(APP_CFG_DB, "mysql_hostname").toString());
    m_ui->m_txtMysqlUsername->lineEdit()->setText(settings->value(APP_CFG_DB, "mysql_username").toString());
    m_ui->m_txtMysqlPassword->lineEdit()->setText(settings->value(APP_CFG_DB, "mysql_password").toString());
    m_ui->m_spinMysqlPort->setValue(settings->value(APP_CFG_DB, "mysql_port", APP_DB_MYSQL_PORT).toInt());
  }

  int index_current_backend = m_ui->m_cmbDatabaseDriver->findData(settings->value(APP_CFG_DB,
                                                                                  "database_driver",
                                                                                  APP_DB_SQLITE_DRIVER).toString());

  if (index_current_backend >= 0) {
    m_ui->m_cmbDatabaseDriver->setCurrentIndex(index_current_backend);
  }
}

void FormSettings::saveDataStorage() {
  // Setup in-memory database status.
  Settings *settings = qApp->settings();

  bool original_inmemory = settings->value(APP_CFG_DB, "use_in_memory_db", false).toBool();
  bool new_inmemory = m_ui->m_checkSqliteUseInMemoryDatabase->isChecked();

  if (original_inmemory != new_inmemory) {
    m_changedDataTexts.append(tr("in-memory database switched"));
  }

  // Save data storage settings.
  QString original_db_driver = settings->value(APP_CFG_DB, "database_driver", APP_DB_SQLITE_DRIVER).toString();
  QString selected_db_driver = m_ui->m_cmbDatabaseDriver->itemData(m_ui->m_cmbDatabaseDriver->currentIndex()).toString();

  // Save SQLite.
  settings->setValue(APP_CFG_DB, "use_in_memory_db", new_inmemory);

  if (QSqlDatabase::isDriverAvailable(APP_DB_MYSQL_DRIVER)) {
    // Save MySQL.
    settings->setValue(APP_CFG_DB, "mysql_hostname", m_ui->m_txtMysqlHostname->lineEdit()->text());
    settings->setValue(APP_CFG_DB, "mysql_username", m_ui->m_txtMysqlUsername->lineEdit()->text());
    settings->setValue(APP_CFG_DB, "mysql_password", m_ui->m_txtMysqlPassword->lineEdit()->text());
    settings->setValue(APP_CFG_DB, "mysql_port", m_ui->m_spinMysqlPort->value());
  }

  settings->setValue(APP_CFG_DB, "database_driver", selected_db_driver);

  if (original_db_driver != selected_db_driver ||
      m_initialSettings.m_mysqlDataStorageChanged) {
    m_changedDataTexts.append(tr("data storage backend changed"));
  }
}

void FormSettings::mysqlTestConnection() {
  DatabaseFactory::MySQLError error_code = DatabaseFactory::instance()->mysqlTestConnection(m_ui->m_txtMysqlHostname->lineEdit()->text(),
                                                                                            m_ui->m_spinMysqlPort->value(),
                                                                                            m_ui->m_txtMysqlUsername->lineEdit()->text(),
                                                                                            m_ui->m_txtMysqlPassword->lineEdit()->text());
  QString interpretation = DatabaseFactory::instance()->mysqlInterpretErrorCode(error_code);

  switch (error_code) {
    case DatabaseFactory::MySQLOk:
      m_ui->m_lblMysqlTestResult->setStatus(WidgetWithStatus::Ok,
                                            interpretation,
                                            interpretation);
      break;

    default:
      m_ui->m_lblMysqlTestResult->setStatus(WidgetWithStatus::Error,
                                            interpretation,
                                            interpretation);
      break;


  }
}

void FormSettings::onMysqlHostnameChanged(const QString &new_hostname) {
  if (new_hostname.isEmpty()) {
    m_ui->m_txtMysqlHostname->setStatus(LineEditWithStatus::Warning,
                                        tr("Hostname is empty."));
  }
  else {
    m_ui->m_txtMysqlHostname->setStatus(LineEditWithStatus::Ok,
                                        tr("Hostname looks ok."));
  }
}

void FormSettings::onMysqlUsernameChanged(const QString &new_username) {
  if (new_username.isEmpty()) {
    m_ui->m_txtMysqlUsername->setStatus(LineEditWithStatus::Warning,
                                        tr("Username is empty."));
  }
  else {
    m_ui->m_txtMysqlUsername->setStatus(LineEditWithStatus::Ok,
                                        tr("Username looks ok."));
  }
}

void FormSettings::onMysqlPasswordChanged(const QString &new_password) {
  if (new_password.isEmpty()) {
    m_ui->m_txtMysqlPassword->setStatus(LineEditWithStatus::Warning,
                                        tr("Password is empty."));
  }
  else {
    m_ui->m_txtMysqlPassword->setStatus(LineEditWithStatus::Ok,
                                        tr("Password looks ok."));
  }
}

void FormSettings::onMysqlDataStorageEdited() {
  m_initialSettings.m_mysqlDataStorageChanged = true;
}

void FormSettings::loadGeneral() {
  m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text().arg(APP_NAME));

  // Load auto-start status.
  SystemFactory::AutoStartStatus autostart_status = SystemFactory::instance()->getAutoStartStatus();
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
  if (m_ui->m_checkAutostart->isChecked()) {
    SystemFactory::instance()->setAutoStartStatus(SystemFactory::Enabled);
  }
  else {
    SystemFactory::instance()->setAutoStartStatus(SystemFactory::Disabled);
  }
}

void FormSettings::loadInterface() {
  Settings *settings = qApp->settings();

  // Load settings of tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    m_ui->m_radioTrayOff->setChecked(!settings->value(APP_CFG_GUI,
                                                      "use_tray_icon",
                                                      true).toBool());
  }
  // Tray icon is not supported on this machine.
  else {
    m_ui->m_radioTrayOff->setText(tr("Disable (Tray icon is not available.)"));
    m_ui->m_radioTrayOff->setChecked(true);
    m_ui->m_grpTray->setDisabled(true);
  }

  m_ui->m_checkHidden->setChecked(settings->value(APP_CFG_GUI,
                                                  "start_hidden",
                                                  false).toBool());
  m_ui->m_checkHideWhenMinimized->setChecked(settings->value(APP_CFG_GUI,
                                                             "hide_when_minimized",
                                                             false).toBool());

  // Load settings of icon theme.
  QString current_theme = IconFactory::instance()->currentIconTheme();

  foreach (const QString &icon_theme_name, IconFactory::instance()->installedIconThemes()) {
    if (icon_theme_name == APP_NO_THEME) {
      // Add just "no theme" on other systems.
      //: Label for disabling icon theme.
      m_ui->m_cmbIconTheme->addItem(tr("no icon theme"),
                                    APP_NO_THEME);
    }
    else {
      m_ui->m_cmbIconTheme->addItem(icon_theme_name,
                                    icon_theme_name);
    }
  }

  // Mark active theme.
  if (current_theme == APP_NO_THEME) {
    // Because "no icon theme" lies at the index 0.
    m_ui->m_cmbIconTheme->setCurrentIndex(0);
  }
  else {
#if QT_VERSION >= 0x050000
    m_ui->m_cmbIconTheme->setCurrentText(current_theme);
#else
    int theme_index = m_ui->m_cmbIconTheme->findText(current_theme);
    if (theme_index >= 0) {
      m_ui->m_cmbIconTheme->setCurrentIndex(theme_index);
    }
#endif
  }

  // Load skin.
  QString selected_skin = SkinFactory::instance()->selectedSkinName();

  foreach (const Skin &skin, SkinFactory::instance()->installedSkins()) {
    QTreeWidgetItem *new_item = new QTreeWidgetItem(QStringList() <<
                                                    skin.m_visibleName <<
                                                    skin.m_version <<
                                                    skin.m_author <<
                                                    skin.m_email);
    new_item->setData(0, Qt::UserRole, QVariant::fromValue(skin));

    // Add this skin and mark it as active if its active now.
    m_ui->m_treeSkins->addTopLevelItem(new_item);

    if (skin.m_baseName == selected_skin) {
      m_ui->m_treeSkins->setCurrentItem(new_item);
      m_ui->m_lblActiveContents->setText(skin.m_visibleName);
    }
  }

  if (m_ui->m_treeSkins->currentItem() == NULL &&
      m_ui->m_treeSkins->topLevelItemCount() > 0) {
    // Currently active skin is NOT available, select another one as selected
    // if possible.
    m_ui->m_treeSkins->setCurrentItem(m_ui->m_treeSkins->topLevelItem(0));
  }

  // Load tab settings.
  m_ui->m_checkCloseTabsMiddleClick->setChecked(settings->value(APP_CFG_GUI,
                                                                "tab_close_mid_button",
                                                                true).toBool());
  m_ui->m_checkCloseTabsDoubleClick->setChecked(settings->value(APP_CFG_GUI,
                                                                "tab_close_double_button",
                                                                true).toBool());
  m_ui->m_checkNewTabDoubleClick->setChecked(settings->value(APP_CFG_GUI,
                                                             "tab_new_double_button",
                                                             true).toBool());
  m_ui->m_hideTabBarIfOneTabVisible->setChecked(settings->value(APP_CFG_GUI,
                                                                "hide_tabbar_one_tab",
                                                                true).toBool());

  // Load toolbar button style.
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Icon only"), Qt::ToolButtonIconOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text only"), Qt::ToolButtonTextOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text beside icon"), Qt::ToolButtonTextBesideIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text under icon"), Qt::ToolButtonTextUnderIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Follow OS style"), Qt::ToolButtonFollowStyle);

  m_ui->m_cmbToolbarButtonStyle->setCurrentIndex(m_ui->m_cmbToolbarButtonStyle->findData(qApp->settings()->value(APP_CFG_GUI,
                                                                                                                 "toolbar_style",
                                                                                                                 Qt::ToolButtonIconOnly).toInt()));

  // Load toolbars.
  m_ui->m_editorFeedsToolbar->loadFromToolBar(FormMain::instance()->tabWidget()->feedMessageViewer()->feedsToolBar());
  m_ui->m_editorMessagesToolbar->loadFromToolBar(FormMain::instance()->tabWidget()->feedMessageViewer()->messagesToolBar());
}

void FormSettings::saveInterface() {
  Settings *settings = qApp->settings();

  // Save toolbar.
  settings->setValue(APP_CFG_GUI,
                     "toolbar_style",
                     m_ui->m_cmbToolbarButtonStyle->itemData(m_ui->m_cmbToolbarButtonStyle->currentIndex()));

  // Save tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    settings->setValue(APP_CFG_GUI, "use_tray_icon",
                       m_ui->m_radioTrayOn->isChecked());

    if (settings->value(APP_CFG_GUI, "use_tray_icon", true).toBool()) {
      SystemTrayIcon::instance()->show();
      FormMain::instance()->tabWidget()->feedMessageViewer()->feedsView()->notifyWithCounts();
    }
    else {
      FormMain::instance()->display();
      SystemTrayIcon::deleteInstance();
    }
  }

  settings->setValue(APP_CFG_GUI, "start_hidden",
                     m_ui->m_checkHidden->isChecked());
  settings->setValue(APP_CFG_GUI,
                     "hide_when_minimized",
                     m_ui->m_checkHideWhenMinimized->isChecked());

  // Save selected icon theme.
  QString selected_icon_theme = m_ui->m_cmbIconTheme->itemData(m_ui->m_cmbIconTheme->currentIndex()).toString();
  QString original_icon_theme = IconFactory::instance()->currentIconTheme();
  IconFactory::instance()->setCurrentIconTheme(selected_icon_theme);

  // Check if icon theme was changed.
  if (selected_icon_theme != original_icon_theme) {
    m_changedDataTexts.append(tr("icon theme changed"));
  }

  // Save and activate new skin.
  if (m_ui->m_treeSkins->selectedItems().size() > 0) {
    Skin active_skin = m_ui->m_treeSkins->currentItem()->data(0, Qt::UserRole).value<Skin>();

    if (SkinFactory::instance()->selectedSkinName() != active_skin.m_baseName) {
      SkinFactory::instance()->setCurrentSkinName(active_skin.m_baseName);
      m_changedDataTexts.append(tr("skin changed"));
    }
  }

  // Save tab settings.
  settings->setValue(APP_CFG_GUI, "tab_close_mid_button",
                     m_ui->m_checkCloseTabsMiddleClick->isChecked());
  settings->setValue(APP_CFG_GUI, "tab_close_double_button",
                     m_ui->m_checkCloseTabsDoubleClick->isChecked());
  settings->setValue(APP_CFG_GUI, "tab_new_double_button",
                     m_ui->m_checkNewTabDoubleClick->isChecked());
  settings->setValue(APP_CFG_GUI, "hide_tabbar_one_tab",
                     m_ui->m_hideTabBarIfOneTabVisible->isChecked());

  m_ui->m_editorFeedsToolbar->saveToolBar();
  m_ui->m_editorMessagesToolbar->saveToolBar();

  FormMain::instance()->tabWidget()->checkTabBarVisibility();
  FormMain::instance()->tabWidget()->feedMessageViewer()->refreshVisualProperties();
}
