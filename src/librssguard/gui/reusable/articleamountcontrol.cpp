// For license of this file, see <project-root-folder>/LICENSE.md.

#include "articleamountcontrol.h"

#include "miscellaneous/application.h"

ArticleAmountControl::ArticleAmountControl(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  m_ui.m_helpIgnoring
    ->setHelpText(tr("Setting any limitations here will instruct %1 to ignore "
                     "some incoming articles. The logic runs AFTER any article filters so even if your article filter "
                     "accepts particular article, it can still subsequently ignored and not added to database.")
                    .arg(QSL(APP_NAME)),
                  false);
  m_ui.m_helpLimit->setHelpText(tr("All excessive articles are removed automatically by the application, usually after "
                                   "particular feed is fetched. Articles are either completely purged (including "
                                   "articles from recycle bin) from internal "
                                   "database or are just moved to recycle bin."),
                                false);

  m_ui.m_spinArticleCount->setSpecialValueText(tr("all articles"));
  m_ui.m_cbArticleLimittingCustomize->setChecked(true);
  m_ui.m_dtDateTimeToAvoid->setEnabled(false);
  m_ui.m_spinHoursAvoid->setEnabled(false);
  m_ui.m_spinHoursAvoid->setMode(TimeSpinBox::Mode::DaysHours);
  m_ui.m_dtDateTimeToAvoid
    ->setDisplayFormat(qApp->localization()->loadedLocale().dateTimeFormat(QLocale::FormatType::ShortFormat));

  // Ignoring articles.
  connect(m_ui.m_cbAddAnyDateArticles, &QCheckBox::toggled, this, &ArticleAmountControl::changed);
  connect(m_ui.m_gbAvoidOldArticles, &QGroupBox::toggled, this, &ArticleAmountControl::changed);
  connect(m_ui.m_dtDateTimeToAvoid, &QDateTimeEdit::dateTimeChanged, this, &ArticleAmountControl::changed);
  connect(m_ui.m_spinHoursAvoid,
          QOverload<double>::of(&TimeSpinBox::valueChanged),
          this,
          &ArticleAmountControl::changed);
  connect(m_ui.m_rbAvoidAbsolute, &QRadioButton::toggled, this, &ArticleAmountControl::changed);
  connect(m_ui.m_rbAvoidRelative, &QRadioButton::toggled, this, &ArticleAmountControl::changed);

  // Limitting articles.
  connect(m_ui.m_spinArticleCount,
          QOverload<int>::of(&QSpinBox::valueChanged),
          this,
          &ArticleAmountControl::updateArticleCountSuffix);
  connect(m_ui.m_cbArticleLimittingCustomize, &QCheckBox::toggled, this, &ArticleAmountControl::changed);
  connect(m_ui.m_spinArticleCount, QOverload<int>::of(&QSpinBox::valueChanged), this, &ArticleAmountControl::changed);
  connect(m_ui.m_cbMoveToBinNoPurge, &QCheckBox::toggled, this, &ArticleAmountControl::changed);
  connect(m_ui.m_cbNoRemoveImportant, &QCheckBox::toggled, this, &ArticleAmountControl::changed);
  connect(m_ui.m_cbNoRemoveUnread, &QCheckBox::toggled, this, &ArticleAmountControl::changed);
}

void ArticleAmountControl::setForAppWideFeatures(bool app_wide, bool batch_edit) {
  if (app_wide) {
    m_ui.m_cbAddAnyDateArticles->setVisible(false);
    m_ui.m_cbArticleLimittingCustomize->setVisible(false);
  }
  else {
    connect(m_ui.m_cbAddAnyDateArticles, &QCheckBox::toggled, m_ui.m_gbAvoidOldArticles, &QGroupBox::setDisabled);
    connect(m_ui.m_cbArticleLimittingCustomize,
            &QCheckBox::toggled,
            m_ui.m_wdgArticleLimittingCustomize,
            &QGroupBox::setEnabled);
  }

  if (batch_edit) {
    // We hook batch selectors.
    m_ui.m_mcbAddAnyDateArticles->addActionWidget(m_ui.m_cbAddAnyDateArticles);
    m_ui.m_mcbAvoidOldArticles->addActionWidget(m_ui.m_wdgAvoidOldArticles);

    m_ui.m_mcbArticleLimittingCustomize->addActionWidget(m_ui.m_cbArticleLimittingCustomize);
    m_ui.m_mcbArticleLimittingSetup->addActionWidget(m_ui.m_wdgArticleLimittingCustomize);
  }
  else {
    // We hide batch selectors.
    for (auto* cb : findChildren<MultiFeedEditCheckBox*>()) {
      cb->hide();
    }
  }
}

void ArticleAmountControl::load(const Feed::ArticleIgnoreLimit& setup, bool always_avoid) {
  // Ignoring articles.
  if (setup.m_dtToAvoid.isValid() && setup.m_dtToAvoid.toMSecsSinceEpoch() > 0) {
    m_ui.m_rbAvoidAbsolute->setChecked(true);
    m_ui.m_dtDateTimeToAvoid->setDateTime(setup.m_dtToAvoid);
  }
  else if (setup.m_hoursToAvoid > 0) {
    m_ui.m_rbAvoidRelative->setChecked(true);
    m_ui.m_spinHoursAvoid->setValue(setup.m_hoursToAvoid);
  }

  if (always_avoid) {
    m_ui.m_gbAvoidOldArticles->setChecked(m_ui.m_rbAvoidAbsolute->isChecked() || m_ui.m_rbAvoidRelative->isChecked());
  }
  else {
    m_ui.m_gbAvoidOldArticles->setChecked(setup.m_avoidOldArticles);
  }

  m_ui.m_cbAddAnyDateArticles->setChecked(setup.m_addAnyArticlesToDb);

  // Limitting articles.
  m_ui.m_cbArticleLimittingCustomize->setChecked(setup.m_customizeLimitting);
  m_ui.m_spinArticleCount->setValue(setup.m_keepCountOfArticles);
  m_ui.m_cbMoveToBinNoPurge->setChecked(setup.m_moveToBinDontPurge);
  m_ui.m_cbNoRemoveImportant->setChecked(setup.m_doNotRemoveStarred);
  m_ui.m_cbNoRemoveUnread->setChecked(setup.m_doNotRemoveUnread);
}

Feed::ArticleIgnoreLimit ArticleAmountControl::save() const {
  Feed::ArticleIgnoreLimit setup;

  // Ignoring articles.
  setup.m_addAnyArticlesToDb = m_ui.m_cbAddAnyDateArticles->isChecked();
  setup.m_avoidOldArticles = m_ui.m_gbAvoidOldArticles->isChecked();

  if (m_ui.m_rbAvoidAbsolute->isChecked()) {
    setup.m_dtToAvoid = m_ui.m_dtDateTimeToAvoid->dateTime();
  }
  else if (m_ui.m_rbAvoidRelative->isChecked()) {
    setup.m_hoursToAvoid = int(m_ui.m_spinHoursAvoid->value());
  }

  // Limitting articles.
  setup.m_customizeLimitting = m_ui.m_cbArticleLimittingCustomize->isChecked();
  setup.m_keepCountOfArticles = m_ui.m_spinArticleCount->value();
  setup.m_moveToBinDontPurge = m_ui.m_cbMoveToBinNoPurge->isChecked();
  setup.m_doNotRemoveStarred = m_ui.m_cbNoRemoveImportant->isChecked();
  setup.m_doNotRemoveUnread = m_ui.m_cbNoRemoveUnread->isChecked();

  return setup;
}

bool isChangeAllowed(MultiFeedEditCheckBox* mcb, bool batch_edit) {
  return !batch_edit || mcb->isChecked();
}

void ArticleAmountControl::saveFeed(Feed* fd, bool batch_edit) const {
  Feed::ArticleIgnoreLimit& art = fd->articleIgnoreLimit();

  if (isChangeAllowed(m_ui.m_mcbAddAnyDateArticles, batch_edit)) {
    art.m_addAnyArticlesToDb = m_ui.m_cbAddAnyDateArticles->isChecked();
  }

  if (isChangeAllowed(m_ui.m_mcbAvoidOldArticles, batch_edit)) {
    if (m_ui.m_gbAvoidOldArticles->isChecked()) {
      if (m_ui.m_rbAvoidAbsolute->isChecked()) {
        art.m_dtToAvoid = m_ui.m_dtDateTimeToAvoid->dateTime();
        art.m_hoursToAvoid = 0;
      }
      else {
        art.m_dtToAvoid = {};
        art.m_hoursToAvoid = m_ui.m_spinHoursAvoid->value();
      }
    }
    else {
      art.m_dtToAvoid = {};
      art.m_hoursToAvoid = 0;
    }
  }

  if (isChangeAllowed(m_ui.m_mcbArticleLimittingCustomize, batch_edit)) {
    art.m_customizeLimitting = m_ui.m_cbArticleLimittingCustomize->isChecked();
  }

  if (isChangeAllowed(m_ui.m_mcbArticleLimittingSetup, batch_edit)) {
    art.m_keepCountOfArticles = m_ui.m_spinArticleCount->value();
    art.m_doNotRemoveStarred = m_ui.m_cbNoRemoveImportant->isChecked();
    art.m_doNotRemoveUnread = m_ui.m_cbNoRemoveUnread->isChecked();
    art.m_moveToBinDontPurge = m_ui.m_cbMoveToBinNoPurge->isChecked();
  }
}

void ArticleAmountControl::updateArticleCountSuffix(int count) {
  m_ui.m_spinArticleCount->setSuffix(QSL(" ") + tr("newest article(s)", nullptr, count));
}
