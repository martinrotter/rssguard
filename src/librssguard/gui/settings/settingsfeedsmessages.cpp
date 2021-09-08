// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsfeedsmessages.h"

#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/guiutilities.h"
#include "gui/messagesview.h"
#include "gui/reusable/timespinbox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"

#include <QFontDialog>
#include <QLocale>
#include <QStringList>

SettingsFeedsMessages::SettingsFeedsMessages(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsFeedsMessages) {
  m_ui->setupUi(this);

  m_ui->m_spinStartupUpdateDelay->setMode(TimeSpinBox::Mode::MinutesSeconds);

  initializeMessageDateFormats();
  GuiUtilities::setLabelAsNotice(*m_ui->label_9, false);

#if defined(USE_WEBENGINE)
  m_ui->m_tabMessages->layout()->removeWidget(m_ui->m_checkDisplayPlaceholders);
  m_ui->m_checkDisplayPlaceholders->hide();

  connect(m_ui->m_cbShowEnclosuresDirectly, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinHeightImageAttachments, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &SettingsFeedsMessages::dirtifySettings);
#else
  m_ui->m_tabMessages->layout()->removeWidget(m_ui->m_cbShowEnclosuresDirectly);
  m_ui->m_cbShowEnclosuresDirectly->hide();

  m_ui->m_tabMessages->layout()->removeWidget(m_ui->m_spinHeightImageAttachments);
  m_ui->m_spinHeightImageAttachments->hide();

  connect(m_ui->m_checkDisplayPlaceholders, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
#endif

  connect(m_ui->m_spinHeightRowsMessages, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &SettingsFeedsMessages::requireRestart);
  connect(m_ui->m_spinHeightRowsFeeds, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &SettingsFeedsMessages::requireRestart);

  connect(m_ui->m_cmbIgnoreContentsChanges, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cbHideCountsIfNoUnread, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkAutoUpdate, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkAutoUpdateOnlyUnfocused, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkDisplayFeedIcons, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkKeppMessagesInTheMiddle, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkMessagesDateTimeFormat, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkMessagesDateTimeFormat, &QCheckBox::toggled, m_ui->m_cmbMessagesDateTimeFormat, &QComboBox::setEnabled);
  connect(m_ui->m_checkRemoveReadMessagesOnExit, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkBringToForegroundAfterMsgOpened, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkUpdateAllFeedsOnStartup, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinAutoUpdateInterval, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinStartupUpdateDelay, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinHeightRowsMessages, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinHeightRowsFeeds, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkAutoUpdate, &QCheckBox::toggled, m_ui->m_spinAutoUpdateInterval, &TimeSpinBox::setEnabled);
  connect(m_ui->m_checkUpdateAllFeedsOnStartup, &QCheckBox::toggled, m_ui->m_spinStartupUpdateDelay, &TimeSpinBox::setEnabled);
  connect(m_ui->m_spinFeedUpdateTimeout, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cmbMessagesDateTimeFormat, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cmbCountsFeedList, &QComboBox::currentTextChanged, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cmbCountsFeedList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkShowTooltips, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);

  connect(m_ui->m_btnChangeMessagesFont, &QPushButton::clicked, this, [&]() {
    changeFont(*m_ui->m_lblMessagesFont);
  });

  connect(m_ui->m_btnChangeFeedListFont, &QPushButton::clicked, this, [&]() {
    changeFont(*m_ui->m_lblFeedListFont);
  });

  connect(m_ui->m_btnChangeMessageListFont, &QPushButton::clicked, this, [&]() {
    changeFont(*m_ui->m_lblMessageListFont);
  });

  if (!m_ui->m_spinFeedUpdateTimeout->suffix().startsWith(' ')) {
    m_ui->m_spinFeedUpdateTimeout->setSuffix(QSL(" ") + m_ui->m_spinFeedUpdateTimeout->suffix());
  }
}

SettingsFeedsMessages::~SettingsFeedsMessages() {
  delete m_ui;
}

void SettingsFeedsMessages::initializeMessageDateFormats() {
  QStringList best_formats;
  const QDateTime current_dt = QDateTime::currentDateTime();
  const QLocale current_locale = qApp->localization()->loadedLocale();
  auto installed_languages = qApp->localization()->installedLanguages();

  for (const Language& lang : qAsConst(installed_languages)) {
    QLocale locale(lang.m_code);

    best_formats << locale.dateTimeFormat(QLocale::FormatType::LongFormat)
                 << locale.dateTimeFormat(QLocale::FormatType::ShortFormat)
                 << locale.dateTimeFormat(QLocale::FormatType::NarrowFormat);
  }

  best_formats.removeDuplicates();

  for (const QString& format : qAsConst(best_formats)) {
    m_ui->m_cmbMessagesDateTimeFormat->addItem(current_locale.toString(current_dt, format), format);
  }
}

void SettingsFeedsMessages::changeFont(QLabel& lbl) {
  bool ok;
  QFont new_font = QFontDialog::getFont(&ok, lbl.font(),
                                        this, tr("Select new font"),
                                        QFontDialog::DontUseNativeDialog);

  if (ok) {
    lbl.setFont(new_font);
    dirtifySettings();
  }
}

void SettingsFeedsMessages::loadSettings() {
  onBeginLoadSettings();

  m_ui->m_spinHeightRowsMessages->setValue(settings()->value(GROUP(GUI), SETTING(GUI::HeightRowMessages)).toInt());
  m_ui->m_spinHeightRowsFeeds->setValue(settings()->value(GROUP(GUI), SETTING(GUI::HeightRowFeeds)).toInt());

  m_ui->m_cbHideCountsIfNoUnread->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::HideCountsIfNoUnread)).toBool());
  m_ui->m_checkDisplayFeedIcons->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::DisplayFeedIconsInList)).toBool());
  m_ui->m_checkBringToForegroundAfterMsgOpened->setChecked(settings()->value(GROUP(Messages),
                                                                             SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally)).toBool());
  m_ui->m_checkKeppMessagesInTheMiddle->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool());
  m_ui->m_checkRemoveReadMessagesOnExit->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::ClearReadOnExit)).toBool());
  m_ui->m_checkAutoUpdate->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateEnabled)).toBool());
  m_ui->m_checkAutoUpdateOnlyUnfocused->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateOnlyUnfocused)).toBool());
  m_ui->m_spinAutoUpdateInterval->setValue(settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateInterval)).toInt());
  m_ui->m_spinFeedUpdateTimeout->setValue(settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt());
  m_ui->m_checkUpdateAllFeedsOnStartup->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateOnStartup)).toBool());
  m_ui->m_spinStartupUpdateDelay->setValue(settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateStartupDelay)).toDouble());
  m_ui->m_cmbCountsFeedList->addItems({ QSL("(%unread)"), QSL("[%unread]"), QSL("%unread/%all"),
                                        QSL("%unread-%all"), QSL("[%unread|%all]") });
  m_ui->m_cmbCountsFeedList->setEditText(settings()->value(GROUP(Feeds), SETTING(Feeds::CountFormat)).toString());
  m_ui->m_checkShowTooltips->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::EnableTooltipsFeedsMessages)).toBool());
  m_ui->m_cmbIgnoreContentsChanges->setChecked(settings()->value(GROUP(Messages),
                                                                 SETTING(Messages::IgnoreContentsChanges)).toBool());

#if !defined (USE_WEBENGINE)
  m_ui->m_checkDisplayPlaceholders->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::DisplayImagePlaceholders)).toBool());
#else
  m_ui->m_spinHeightImageAttachments->setValue(settings()->value(GROUP(Messages),
                                                                 SETTING(Messages::MessageHeadImageHeight)).toInt());
  m_ui->m_cbShowEnclosuresDirectly->setChecked(settings()->value(GROUP(Messages),
                                                                 SETTING(Messages::DisplayEnclosuresInMessage)).toBool());
#endif

  m_ui->m_checkMessagesDateTimeFormat->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::UseCustomDate)).toBool());
  const int index_format = m_ui->m_cmbMessagesDateTimeFormat->findData(settings()->value(GROUP(Messages),
                                                                                         SETTING(Messages::CustomDateFormat)).toString());

  if (index_format >= 0) {
    m_ui->m_cmbMessagesDateTimeFormat->setCurrentIndex(index_format);
  }

  QFont fon;

  fon.fromString(settings()->value(GROUP(Messages),
                                   SETTING(Messages::PreviewerFontStandard)).toString());
  m_ui->m_lblMessagesFont->setFont(fon);

  QFont fon2;

  // Keep in sync with void MessagesModel::setupFonts().
  fon2.fromString(settings()->value(GROUP(Messages),
                                    Messages::ListFont,
                                    Application::font("MessagesView").toString()).toString());
  m_ui->m_lblMessageListFont->setFont(fon2);

  QFont fon3;

  // Keep in sync with void FeedsModel::setupFonts().
  fon3.fromString(settings()->value(GROUP(Feeds),
                                    Feeds::ListFont,
                                    Application::font("FeedsView").toString()).toString());
  m_ui->m_lblFeedListFont->setFont(fon3);

  onEndLoadSettings();
}

void SettingsFeedsMessages::saveSettings() {
  onBeginSaveSettings();

  settings()->setValue(GROUP(GUI), GUI::HeightRowMessages, m_ui->m_spinHeightRowsMessages->value());
  settings()->setValue(GROUP(GUI), GUI::HeightRowFeeds, m_ui->m_spinHeightRowsFeeds->value());

  settings()->setValue(GROUP(Feeds), Feeds::HideCountsIfNoUnread, m_ui->m_cbHideCountsIfNoUnread->isChecked());
  settings()->setValue(GROUP(Messages), Messages::DisplayFeedIconsInList, m_ui->m_checkDisplayFeedIcons->isChecked());
  settings()->setValue(GROUP(Messages), Messages::BringAppToFrontAfterMessageOpenedExternally,
                       m_ui->m_checkBringToForegroundAfterMsgOpened->isChecked());
  settings()->setValue(GROUP(Messages), Messages::KeepCursorInCenter, m_ui->m_checkKeppMessagesInTheMiddle->isChecked());
  settings()->setValue(GROUP(Messages), Messages::ClearReadOnExit, m_ui->m_checkRemoveReadMessagesOnExit->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::AutoUpdateEnabled, m_ui->m_checkAutoUpdate->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::AutoUpdateOnlyUnfocused, m_ui->m_checkAutoUpdateOnlyUnfocused->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::AutoUpdateInterval, m_ui->m_spinAutoUpdateInterval->value());
  settings()->setValue(GROUP(Feeds), Feeds::UpdateTimeout, m_ui->m_spinFeedUpdateTimeout->value());
  settings()->setValue(GROUP(Feeds), Feeds::FeedsUpdateOnStartup, m_ui->m_checkUpdateAllFeedsOnStartup->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::FeedsUpdateStartupDelay, m_ui->m_spinStartupUpdateDelay->value());
  settings()->setValue(GROUP(Feeds), Feeds::CountFormat, m_ui->m_cmbCountsFeedList->currentText());
  settings()->setValue(GROUP(Messages), Messages::UseCustomDate, m_ui->m_checkMessagesDateTimeFormat->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::EnableTooltipsFeedsMessages, m_ui->m_checkShowTooltips->isChecked());
  settings()->setValue(GROUP(Messages), Messages::IgnoreContentsChanges, m_ui->m_cmbIgnoreContentsChanges->isChecked());

#if !defined (USE_WEBENGINE)
  settings()->setValue(GROUP(Messages), Messages::DisplayImagePlaceholders, m_ui->m_checkDisplayPlaceholders->isChecked());
#else
  settings()->setValue(GROUP(Messages), Messages::MessageHeadImageHeight, m_ui->m_spinHeightImageAttachments->value());
  settings()->setValue(GROUP(Messages),
                       Messages::DisplayEnclosuresInMessage,
                       m_ui->m_cbShowEnclosuresDirectly->isChecked());
#endif

  settings()->setValue(GROUP(Messages), Messages::CustomDateFormat,
                       m_ui->m_cmbMessagesDateTimeFormat->itemData(m_ui->m_cmbMessagesDateTimeFormat->currentIndex()).toString());

  // Save fonts.
  settings()->setValue(GROUP(Messages), Messages::PreviewerFontStandard, m_ui->m_lblMessagesFont->font().toString());
  settings()->setValue(GROUP(Messages), Messages::ListFont, m_ui->m_lblMessageListFont->font().toString());
  settings()->setValue(GROUP(Feeds), Feeds::ListFont, m_ui->m_lblFeedListFont->font().toString());

  qApp->mainForm()->tabWidget()->feedMessageViewer()->loadMessageViewerFonts();

  qApp->feedReader()->updateAutoUpdateStatus();
  qApp->feedReader()->feedsModel()->reloadWholeLayout();

  qApp->feedReader()->messagesModel()->updateDateFormat();
  qApp->feedReader()->messagesModel()->updateFeedIconsDisplay();
  qApp->feedReader()->messagesModel()->reloadWholeLayout();

  onEndSaveSettings();
}
