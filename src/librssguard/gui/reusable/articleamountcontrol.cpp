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
  m_ui.m_helpLimit->setHelpText(tr("All excessive articles are removed automatically by the application, usually when "
                                   "particular feed is fetched. Articles are either completely purged from internal "
                                   "database or are just moved to recycle bin."),
                                false);

  m_ui.m_spinArticleCount->setSpecialValueText(tr("all articles"));
  m_ui.m_cbAddAnyDateArticles->setChecked(true);
  m_ui.m_dtDateTimeToAvoid->setEnabled(false);
  m_ui.m_spinHoursAvoid->setEnabled(false);
  m_ui.m_spinHoursAvoid->setMode(TimeSpinBox::Mode::DaysHours);
  m_ui.m_dtDateTimeToAvoid
    ->setDisplayFormat(qApp->localization()->loadedLocale().dateTimeFormat(QLocale::FormatType::ShortFormat));

  connect(m_ui.m_cbAddAnyDateArticles, &QCheckBox::toggled, this, [this](bool checked) {
    m_ui.m_gbAvoidOldArticles->setEnabled(!checked);
  });

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
  connect(m_ui.m_spinArticleCount, &QSpinBox::valueChanged, this, &ArticleAmountControl::changed);
  connect(m_ui.m_cbMoveToBinNoPurge, &QCheckBox::toggled, this, &ArticleAmountControl::changed);
  connect(m_ui.m_cbNoRemoveImportant, &QCheckBox::toggled, this, &ArticleAmountControl::changed);
  connect(m_ui.m_cbNoRemoveUnread, &QCheckBox::toggled, this, &ArticleAmountControl::changed);
}

void ArticleAmountControl::setForAppWideFeatures(bool app_wide, bool batch_edit) {
  if (app_wide) {
    m_ui.m_cbAddAnyDateArticles->setVisible(false);
  }
  else {
    connect(m_ui.m_cbAddAnyDateArticles, &QCheckBox::toggled, m_ui.m_gbAvoidOldArticles, &QGroupBox::setEnabled);
  }

  if (batch_edit) {
    // We hook batch selectors.
    m_ui.m_mcbAddAnyDateArticles->addActionWidget(m_ui.m_cbAddAnyDateArticles);
    m_ui.m_mcbAvoidOldArticles->addActionWidget(m_ui.m_wdgAvoidOldArticles);

    m_ui.m_mcbArticleCount->addActionWidget(m_ui.m_spinArticleCount);
    m_ui.m_mcbMoveToBinNoPurge->addActionWidget(m_ui.m_cbMoveToBinNoPurge);
    m_ui.m_mcbNoRemoveImportant->addActionWidget(m_ui.m_cbNoRemoveImportant);
    m_ui.m_mcbNoRemoveUnread->addActionWidget(m_ui.m_cbNoRemoveUnread);
  }
  else {
    // We hide batch selectors.
    for (auto* cb : findChildren<MultiFeedEditCheckBox*>()) {
      cb->hide();
    }
  }
}

void ArticleAmountControl::load(const Setup& setup) {
  // Ignoring articles.
  if (setup.m_dtToAvoid.isValid() && setup.m_dtToAvoid.toMSecsSinceEpoch() > 0) {
    m_ui.m_rbAvoidAbsolute->setChecked(true);
    m_ui.m_dtDateTimeToAvoid->setDateTime(setup.m_dtToAvoid);
  }
  else {
    m_ui.m_rbAvoidRelative->setChecked(true);
    m_ui.m_spinHoursAvoid->setValue(setup.m_hoursToAvoid);
  }

  m_ui.m_gbAvoidOldArticles->setChecked(setup.m_avoidOldArticles);

  // Limitting articles.
  m_ui.m_spinArticleCount->setValue(setup.m_keepCountOfArticles);
  m_ui.m_cbMoveToBinNoPurge->setChecked(setup.m_moveToBinDontPurge);
  m_ui.m_cbNoRemoveImportant->setChecked(setup.m_doNotRemoveStarred);
  m_ui.m_cbNoRemoveUnread->setChecked(setup.m_doNotRemoveUnread);
}

ArticleAmountControl::Setup ArticleAmountControl::save() const {
  Setup setup;

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
  setup.m_keepCountOfArticles = m_ui.m_spinArticleCount->value();
  setup.m_moveToBinDontPurge = m_ui.m_cbMoveToBinNoPurge->isChecked();
  setup.m_doNotRemoveStarred = m_ui.m_cbNoRemoveImportant->isChecked();
  setup.m_doNotRemoveUnread = m_ui.m_cbNoRemoveUnread->isChecked();

  return setup;
}

bool isChangeAllowed(MultiFeedEditCheckBox* mcb) {
  return mcb->isChecked();
}

void ArticleAmountControl::saveFeed(Feed* fd) const {
  if (isChangeAllowed(m_ui.m_mcbAddAnyDateArticles)) {
    fd->setAddAnyDatetimeArticles(m_ui.m_cbAddAnyDateArticles->isChecked());
  }

  if (isChangeAllowed(m_ui.m_mcbAvoidOldArticles)) {
    if (m_ui.m_gbAvoidOldArticles->isChecked()) {
      if (m_ui.m_rbAvoidAbsolute->isChecked()) {
        fd->setDatetimeToAvoid(m_ui.m_dtDateTimeToAvoid->dateTime());
        fd->setHoursToAvoid(0);
      }
      else {
        fd->setDatetimeToAvoid({});
        fd->setHoursToAvoid(m_ui.m_spinHoursAvoid->value());
      }
    }
    else {
      fd->setDatetimeToAvoid({});
      fd->setHoursToAvoid(0);
    }
  }
}

void ArticleAmountControl::updateArticleCountSuffix(int count) {
  m_ui.m_spinArticleCount->setSuffix(QSL(" ") + tr("newest article(s)", nullptr, count));
}
