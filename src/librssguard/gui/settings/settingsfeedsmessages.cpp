// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsfeedsmessages.h"

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/messagebox.h"
#include "gui/reusable/timespinbox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"

#include <QFontDialog>
#include <QLocale>
#include <QMetaEnum>
#include <QStringList>

SettingsFeedsMessages::SettingsFeedsMessages(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsFeedsMessages) {
  m_ui->setupUi(this);

  m_ui->m_spinHoursAvoid->setMode(TimeSpinBox::Mode::DaysHours);
  m_ui->m_spinAutoUpdateInterval->setMode(TimeSpinBox::Mode::MinutesSeconds);
  m_ui->m_spinStartupUpdateDelay->setMode(TimeSpinBox::Mode::MinutesSeconds);

  initializeMessageDateFormats();

  m_ui->m_helpCountsFeedsFormat
    ->setHelpText(tr("Enter format for count of articles displayed next to each "
                     "feed/category in feed list. Use \"%all\" and \"%unread\" strings "
                     "which are placeholders for the actual count of all (or unread) articles."),
                  false);
  m_ui->m_helpMultilineArticleList->setHelpText(tr("Note that enabling this might have drastic consequences on "
                                                   "performance of article list with big number of articles."),
                                                true);

  QMetaEnum enumer = QMetaEnum::fromType<MessagesModel::MessageUnreadIcon>();

  for (int i = 0; i < enumer.keyCount(); i++) {
    auto en = MessagesModel::MessageUnreadIcon(enumer.value(i));

    m_ui->m_cmbUnreadIconType->addItem(MessagesModel::descriptionOfUnreadIcon(en), int(en));
  }

  connect(m_ui->m_cbShowEnclosuresDirectly, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinHeightImageAttachments,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);

  connect(m_ui->m_spinRelativeArticleTime, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
    if (value <= 0) {
      m_ui->m_spinRelativeArticleTime->setSuffix(QSL(" ") + tr("days (turned off)"));
    }
    else {
      m_ui->m_spinRelativeArticleTime->setSuffix(QSL(" ") + tr("day(s)", nullptr, value));
    }
  });

  connect(m_ui->m_spinHeightImageAttachments, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
    if (value <= 0) {
      m_ui->m_spinHeightImageAttachments->setSuffix(QSL(" px") + tr(" = unchanged size"));
    }
    else {
      m_ui->m_spinHeightImageAttachments->setSuffix(QSL(" px"));
    }
  });

  connect(m_ui->m_gbFeedListFont, &QGroupBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_gbArticleListFont, &QGroupBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cbListsRestrictedShortcuts, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cmbIgnoreContentsChanges, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cbHideCountsIfNoUnread, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkAutoUpdate, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cbUpdateFeedListDuringFetching, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkAutoUpdateOnlyUnfocused, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cmbUnreadIconType,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cmbUnreadIconType,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &SettingsFeedsMessages::requireRestart);

  connect(m_ui->m_checkKeppMessagesInTheMiddle, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cbArticleViewerAlwaysVisible, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkMessagesDateTimeFormat, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkMessagesDateTimeFormat,
          &QCheckBox::toggled,
          m_ui->m_cmbMessagesDateTimeFormat,
          &QComboBox::setEnabled);

  connect(m_ui->m_checkMessagesTimeFormat, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cmbFastAutoUpdate, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkMessagesTimeFormat, &QCheckBox::toggled, m_ui->m_cmbMessagesTimeFormat, &QComboBox::setEnabled);

  connect(m_ui->m_checkRemoveReadMessagesOnExit, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkBringToForegroundAfterMsgOpened,
          &QCheckBox::toggled,
          this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkUpdateAllFeedsOnStartup, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinAutoUpdateInterval,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinStartupUpdateDelay,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);

  connect(m_ui->m_spinHeightRowsMessages,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinHeightRowsMessages,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::requireRestart);

  connect(m_ui->m_spinHeightRowsFeeds,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinHeightRowsFeeds,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::requireRestart);

  connect(m_ui->m_spinPaddingRowsMessages,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinPaddingRowsMessages,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::requireRestart);

  connect(m_ui->m_spinRelativeArticleTime,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);

  connect(m_ui->m_checkAutoUpdate, &QCheckBox::toggled, m_ui->m_spinAutoUpdateInterval, &TimeSpinBox::setEnabled);
  connect(m_ui->m_checkUpdateAllFeedsOnStartup,
          &QCheckBox::toggled,
          m_ui->m_spinStartupUpdateDelay,
          &TimeSpinBox::setEnabled);
  connect(m_ui->m_spinFeedUpdateTimeout,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);

  connect(m_ui->m_cmbMessagesDateTimeFormat,
          &QComboBox::currentTextChanged,
          this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cmbMessagesTimeFormat, &QComboBox::currentTextChanged, this, &SettingsFeedsMessages::dirtifySettings);

  connect(m_ui->m_cbFixupArticleDatetime, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);

  connect(m_ui->m_cmbCountsFeedList, &QComboBox::currentTextChanged, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_cmbCountsFeedList,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkShowTooltips, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkMultilineArticleList, &QCheckBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_checkMultilineArticleList, &QCheckBox::toggled, this, &SettingsFeedsMessages::requireRestart);

  connect(m_ui->m_cmbMessagesDateTimeFormat,
          &QComboBox::currentTextChanged,
          this,
          &SettingsFeedsMessages::updateDateTimeTooltip);
  connect(m_ui->m_cmbMessagesTimeFormat,
          &QComboBox::currentTextChanged,
          this,
          &SettingsFeedsMessages::updateDateTimeTooltip);

  emit m_ui->m_cmbMessagesDateTimeFormat->currentTextChanged({});
  emit m_ui->m_cmbMessagesTimeFormat->currentTextChanged({});

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

  m_ui->m_dtDateTimeToAvoid
    ->setDisplayFormat(qApp->localization()->loadedLocale().dateTimeFormat(QLocale::FormatType::ShortFormat));

  connect(m_ui->m_gbAvoidOldArticles, &QGroupBox::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_dtDateTimeToAvoid, &QDateTimeEdit::dateTimeChanged, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_spinHoursAvoid, &TimeSpinBox::valueChanged, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_rbAvoidAbsolute, &QRadioButton::toggled, this, &SettingsFeedsMessages::dirtifySettings);
  connect(m_ui->m_rbAvoidAbsolute, &QRadioButton::toggled, this, &SettingsFeedsMessages::dirtifySettings);

  m_ui->m_spinRelativeArticleTime->setValue(-1);
}

SettingsFeedsMessages::~SettingsFeedsMessages() {
  delete m_ui;
}

QIcon SettingsFeedsMessages::icon() const {
  return qApp->icons()->fromTheme(QSL("mail-mark-read"));
}

void SettingsFeedsMessages::initializeMessageDateFormats() {
  QStringList patterns = TextFactory::dateTimePatterns();

  m_ui->m_cmbMessagesDateTimeFormat->addItems(patterns);
  m_ui->m_cmbMessagesTimeFormat->addItems(patterns);

  for (int i = 0; i < patterns.size(); i++) {
    m_ui->m_cmbMessagesDateTimeFormat->setItemData(i,
                                                   QDateTime::currentDateTime().toString(patterns.at(i)),
                                                   Qt::ItemDataRole::ToolTipRole);
    m_ui->m_cmbMessagesTimeFormat->setItemData(i,
                                               QDateTime::currentDateTime().toString(patterns.at(i)),
                                               Qt::ItemDataRole::ToolTipRole);
  }
}

void SettingsFeedsMessages::changeFont(QLabel& lbl) {
  bool ok;
  QFont new_font = QFontDialog::getFont(&ok, lbl.font(), this, tr("Select new font"), QFontDialog::DontUseNativeDialog);

  if (ok) {
    lbl.setFont(new_font);
    dirtifySettings();
  }
}

void SettingsFeedsMessages::loadSettings() {
  onBeginLoadSettings();

  m_ui->m_spinRelativeArticleTime
    ->setValue(settings()->value(GROUP(Messages), SETTING(Messages::RelativeTimeForNewerArticles)).toInt());
  m_ui->m_spinPaddingRowsMessages
    ->setValue(settings()->value(GROUP(Messages), SETTING(Messages::ArticleListPadding)).toInt());
  m_ui->m_spinHeightRowsMessages->setValue(settings()->value(GROUP(GUI), SETTING(GUI::HeightRowMessages)).toInt());
  m_ui->m_spinHeightRowsFeeds->setValue(settings()->value(GROUP(GUI), SETTING(GUI::HeightRowFeeds)).toInt());

  m_ui->m_cbUpdateFeedListDuringFetching
    ->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateFeedListDuringFetching)).toBool());
  m_ui->m_cbListsRestrictedShortcuts
    ->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::OnlyBasicShortcutsInLists)).toBool());
  m_ui->m_cbHideCountsIfNoUnread
    ->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::HideCountsIfNoUnread)).toBool());
  m_ui->m_cmbUnreadIconType
    ->setCurrentIndex(m_ui->m_cmbUnreadIconType
                        ->findData(settings()->value(GROUP(Messages), SETTING(Messages::UnreadIconType)).toInt()));
  m_ui->m_checkBringToForegroundAfterMsgOpened
    ->setChecked(settings()
                   ->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally))
                   .toBool());
  m_ui->m_checkKeppMessagesInTheMiddle
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::KeepCursorInCenter)).toBool());
  m_ui->m_checkRemoveReadMessagesOnExit
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::ClearReadOnExit)).toBool());
  m_ui->m_checkAutoUpdate->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateEnabled)).toBool());
  m_ui->m_checkAutoUpdateOnlyUnfocused
    ->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateOnlyUnfocused)).toBool());
  m_ui->m_spinAutoUpdateInterval->setValue(settings()->value(GROUP(Feeds), SETTING(Feeds::AutoUpdateInterval)).toInt());
  m_ui->m_spinFeedUpdateTimeout->setValue(settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt());

  m_ui->m_dtDateTimeToAvoid->setEnabled(false);
  m_ui->m_spinHoursAvoid->setEnabled(false);

  QDateTime avoid_dt_barrier =
    settings()->value(GROUP(Messages), SETTING(Messages::DateTimeToAvoidArticle)).toDateTime();
  int avoid_hour_barrier = settings()->value(GROUP(Messages), SETTING(Messages::HoursToAvoidArticle)).toInt();

  if (avoid_dt_barrier.isValid() && avoid_dt_barrier.toMSecsSinceEpoch() > 0) {
    m_ui->m_rbAvoidAbsolute->setChecked(true);
    m_ui->m_dtDateTimeToAvoid
      ->setDateTime(settings()->value(GROUP(Messages), SETTING(Messages::DateTimeToAvoidArticle)).toDateTime());
  }
  else {
    m_ui->m_rbAvoidRelative->setChecked(true);
    m_ui->m_spinHoursAvoid->setValue(avoid_hour_barrier);
  }

  m_ui->m_gbAvoidOldArticles
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::AvoidOldArticles)).toBool());

  m_ui->m_cmbFastAutoUpdate->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::FastAutoUpdate)).toBool());
  m_ui->m_checkUpdateAllFeedsOnStartup
    ->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateOnStartup)).toBool());
  m_ui->m_spinStartupUpdateDelay
    ->setValue(settings()->value(GROUP(Feeds), SETTING(Feeds::FeedsUpdateStartupDelay)).toDouble());
  m_ui->m_cmbCountsFeedList
    ->addItems({QSL("(%unread)"), QSL("[%unread]"), QSL("%unread/%all"), QSL("%unread-%all"), QSL("[%unread|%all]")});
  m_ui->m_cmbCountsFeedList->setEditText(settings()->value(GROUP(Feeds), SETTING(Feeds::CountFormat)).toString());
  m_ui->m_checkShowTooltips
    ->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::EnableTooltipsFeedsMessages)).toBool());
  m_ui->m_cmbIgnoreContentsChanges
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::IgnoreContentsChanges)).toBool());
  m_ui->m_checkMultilineArticleList
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::MultilineArticleList)).toBool());

  m_ui->m_cbArticleViewerAlwaysVisible
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::AlwaysDisplayItemPreview)).toBool());
  m_ui->m_spinHeightImageAttachments
    ->setValue(settings()->value(GROUP(Messages), SETTING(Messages::LimitArticleImagesHeight)).toInt());
  m_ui->m_cbShowEnclosuresDirectly
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::DisplayEnclosuresInMessage)).toBool());

  m_ui->m_cbFixupArticleDatetime
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::FixupFutureArticleDateTimes)).toBool());

  m_ui->m_checkMessagesDateTimeFormat
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::UseCustomDate)).toBool());
  m_ui->m_cmbMessagesDateTimeFormat
    ->setCurrentText(settings()->value(GROUP(Messages), SETTING(Messages::CustomDateFormat)).toString());

  m_ui->m_checkMessagesTimeFormat
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::UseCustomTime)).toBool());
  m_ui->m_cmbMessagesTimeFormat
    ->setCurrentText(settings()->value(GROUP(Messages), SETTING(Messages::CustomTimeFormat)).toString());

  QFont fon;

  fon.fromString(settings()->value(GROUP(Messages), SETTING(Messages::PreviewerFontStandard)).toString());
  m_ui->m_lblMessagesFont->setFont(fon);

  QFont fon2;

  // Keep in sync with void MessagesModel::setupFonts().
  fon2.fromString(settings()
                    ->value(GROUP(Messages), Messages::ListFont, Application::font("MessagesView").toString())
                    .toString());
  m_ui->m_lblMessageListFont->setFont(fon2);
  m_ui->m_gbArticleListFont
    ->setChecked(settings()->value(GROUP(Messages), SETTING(Messages::CustomizeListFont)).toBool());

  QFont fon3;

  // Keep in sync with void FeedsModel::setupFonts().
  fon3
    .fromString(settings()->value(GROUP(Feeds), Feeds::ListFont, Application::font("FeedsView").toString()).toString());
  m_ui->m_lblFeedListFont->setFont(fon3);
  m_ui->m_gbFeedListFont->setChecked(settings()->value(GROUP(Feeds), SETTING(Feeds::CustomizeListFont)).toBool());

  onEndLoadSettings();
}

void SettingsFeedsMessages::saveSettings() {
  onBeginSaveSettings();

  settings()->setValue(GROUP(Messages),
                       Messages::RelativeTimeForNewerArticles,
                       m_ui->m_spinRelativeArticleTime->value());
  settings()->setValue(GROUP(Messages), Messages::ArticleListPadding, m_ui->m_spinPaddingRowsMessages->value());
  settings()->setValue(GROUP(GUI), GUI::HeightRowMessages, m_ui->m_spinHeightRowsMessages->value());
  settings()->setValue(GROUP(GUI), GUI::HeightRowFeeds, m_ui->m_spinHeightRowsFeeds->value());

  settings()->setValue(GROUP(Feeds),
                       Feeds::UpdateFeedListDuringFetching,
                       m_ui->m_cbUpdateFeedListDuringFetching->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::OnlyBasicShortcutsInLists, m_ui->m_cbListsRestrictedShortcuts->isChecked());

  settings()->setValue(GROUP(Feeds), Feeds::HideCountsIfNoUnread, m_ui->m_cbHideCountsIfNoUnread->isChecked());
  settings()->setValue(GROUP(Messages), Messages::UnreadIconType, m_ui->m_cmbUnreadIconType->currentData().toInt());

  // Make registry hack to make "show app window after external viewer called" feature
  // work more reliably on Windows 10+.
#if defined(Q_OS_WIN)
  if (!settings()->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally)).toBool() &&
      m_ui->m_checkBringToForegroundAfterMsgOpened->isChecked()) {
    QSettings registry_key(QSL("HKEY_CURRENT_USER\\Control Panel\\Desktop"), QSettings::Format::NativeFormat);

    registry_key.setValue(QSL("ForegroundFlashCount"), 3);
    registry_key.setValue(QSL("ForegroundLockTimeout"), 0);

    MsgBox::show(this,
                 QMessageBox::Icon::Warning,
                 tr("PC restart needed"),
                 tr("Your PC needs to be restarted to make some of enabled features fully working."));
  }
#endif

  settings()->setValue(GROUP(Messages),
                       Messages::BringAppToFrontAfterMessageOpenedExternally,
                       m_ui->m_checkBringToForegroundAfterMsgOpened->isChecked());

  settings()->setValue(GROUP(Messages),
                       Messages::KeepCursorInCenter,
                       m_ui->m_checkKeppMessagesInTheMiddle->isChecked());
  settings()->setValue(GROUP(Messages), Messages::ClearReadOnExit, m_ui->m_checkRemoveReadMessagesOnExit->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::AutoUpdateEnabled, m_ui->m_checkAutoUpdate->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::AutoUpdateOnlyUnfocused, m_ui->m_checkAutoUpdateOnlyUnfocused->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::AutoUpdateInterval, m_ui->m_spinAutoUpdateInterval->value());
  settings()->setValue(GROUP(Feeds), Feeds::UpdateTimeout, m_ui->m_spinFeedUpdateTimeout->value());

  settings()->setValue(GROUP(Messages), Messages::AvoidOldArticles, m_ui->m_gbAvoidOldArticles->isChecked());

  if (m_ui->m_rbAvoidAbsolute->isChecked()) {
    settings()->setValue(GROUP(Messages), Messages::DateTimeToAvoidArticle, m_ui->m_dtDateTimeToAvoid->dateTime());
    settings()->setValue(GROUP(Messages), Messages::HoursToAvoidArticle, 0);
  }
  else if (m_ui->m_rbAvoidRelative->isChecked()) {
    settings()->setValue(GROUP(Messages), Messages::DateTimeToAvoidArticle, QDateTime());
    settings()->setValue(GROUP(Messages), Messages::HoursToAvoidArticle, m_ui->m_spinHoursAvoid->value());
  }

  settings()->setValue(GROUP(Feeds), Feeds::FastAutoUpdate, m_ui->m_cmbFastAutoUpdate->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::FeedsUpdateOnStartup, m_ui->m_checkUpdateAllFeedsOnStartup->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::FeedsUpdateStartupDelay, m_ui->m_spinStartupUpdateDelay->value());
  settings()->setValue(GROUP(Feeds), Feeds::CountFormat, m_ui->m_cmbCountsFeedList->currentText());
  settings()->setValue(GROUP(Feeds), Feeds::EnableTooltipsFeedsMessages, m_ui->m_checkShowTooltips->isChecked());
  settings()->setValue(GROUP(Messages), Messages::IgnoreContentsChanges, m_ui->m_cmbIgnoreContentsChanges->isChecked());
  settings()->setValue(GROUP(Messages), Messages::MultilineArticleList, m_ui->m_checkMultilineArticleList->isChecked());
  settings()->setValue(GROUP(Messages),
                       Messages::LimitArticleImagesHeight,
                       m_ui->m_spinHeightImageAttachments->value());
  settings()->setValue(GROUP(Messages),
                       Messages::DisplayEnclosuresInMessage,
                       m_ui->m_cbShowEnclosuresDirectly->isChecked());

  settings()->setValue(GROUP(Messages),
                       Messages::FixupFutureArticleDateTimes,
                       m_ui->m_cbFixupArticleDatetime->isChecked());

  settings()->setValue(GROUP(Messages),
                       Messages::AlwaysDisplayItemPreview,
                       m_ui->m_cbArticleViewerAlwaysVisible->isChecked());
  settings()->setValue(GROUP(Messages), Messages::UseCustomDate, m_ui->m_checkMessagesDateTimeFormat->isChecked());
  settings()->setValue(GROUP(Messages), Messages::UseCustomTime, m_ui->m_checkMessagesTimeFormat->isChecked());

  settings()->setValue(GROUP(Messages), Messages::CustomDateFormat, m_ui->m_cmbMessagesDateTimeFormat->currentText());
  settings()->setValue(GROUP(Messages), Messages::CustomTimeFormat, m_ui->m_cmbMessagesTimeFormat->currentText());

  // Save fonts.
  settings()->setValue(GROUP(Messages), Messages::PreviewerFontStandard, m_ui->m_lblMessagesFont->font().toString());
  settings()->setValue(GROUP(Messages), Messages::ListFont, m_ui->m_lblMessageListFont->font().toString());
  settings()->setValue(GROUP(Feeds), Feeds::ListFont, m_ui->m_lblFeedListFont->font().toString());

  settings()->setValue(GROUP(Messages), Messages::CustomizeListFont, m_ui->m_gbArticleListFont->isChecked());
  settings()->setValue(GROUP(Feeds), Feeds::CustomizeListFont, m_ui->m_gbFeedListFont->isChecked());

  qApp->mainForm()->tabWidget()->feedMessageViewer()->updateArticleViewerSettings();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->loadMessageViewerFonts();

  qApp->feedReader()->updateAutoUpdateStatus();
  qApp->feedReader()->feedsModel()->setupBehaviorDuringFetching();
  qApp->feedReader()->feedsModel()->reloadWholeLayout();

  qApp->feedReader()->messagesModel()->updateDateFormat();
  qApp->feedReader()->messagesModel()->reloadWholeLayout();

  onEndSaveSettings();
}

void SettingsFeedsMessages::updateDateTimeTooltip() {
  QComboBox* sndr = qobject_cast<QComboBox*>(sender());

  if (sndr != nullptr) {
    if (sndr->currentText().simplified().isEmpty()) {
      sndr->setToolTip({});
    }
    else {
      sndr->setToolTip(QDateTime::currentDateTime().toString(sndr->currentText()));
    }
  }
}
