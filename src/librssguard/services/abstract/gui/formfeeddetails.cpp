// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formfeeddetails.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/guiutilities.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/gui/multifeededitcheckbox.h"
#include "services/abstract/rootitem.h"

#include <QMenu>
#include <QNetworkReply>
#include <QPair>
#include <QPushButton>
#include <QTextCodec>

FormFeedDetails::FormFeedDetails(ServiceRoot* service_root, QWidget* parent)
  : QDialog(parent), m_serviceRoot(service_root) {
  initialize();
  createConnections();

  m_ui.m_mcbAutoDownloading->addActionWidget(m_ui.m_cmbAutoUpdateType);
  m_ui.m_mcbAutoDownloading->addActionWidget(m_ui.m_spinAutoUpdateInterval);
  m_ui.m_mcbAddAnyDateArticles->addActionWidget(m_ui.m_cbAddAnyDateArticles);
  m_ui.m_mcbOpenArticlesAutomatically->addActionWidget(m_ui.m_cbOpenArticlesAutomatically);
  m_ui.m_mcbAvoidOldArticles->addActionWidget(m_ui.m_gbAvoidOldArticles);

  m_ui.m_mcbDisableFeed->addActionWidget(m_ui.m_cbDisableFeed);
  m_ui.m_mcbSuppressFeed->addActionWidget(m_ui.m_cbSuppressFeed);
  m_ui.m_mcbFeedRtl->addActionWidget(m_ui.m_cbFeedRTL);
}

void FormFeedDetails::activateTab(int index) {
  m_ui.m_tabWidget->setCurrentIndex(index);
}

void FormFeedDetails::clearTabs() {
  m_ui.m_tabWidget->clear();
}

void FormFeedDetails::insertCustomTab(QWidget* custom_tab, const QString& title, int index) {
  m_ui.m_tabWidget->insertTab(index, custom_tab, title);
}

void FormFeedDetails::apply() {
  QList<Feed*> fds = feeds<Feed>();

  for (Feed* fd : fds) {
    // Setup common data for the feed.
    if (isChangeAllowed(m_ui.m_mcbAutoDownloading)) {
      fd->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(m_ui.m_cmbAutoUpdateType
                                                                ->itemData(m_ui.m_cmbAutoUpdateType->currentIndex())
                                                                .toInt()));
      fd->setAutoUpdateInterval(int(m_ui.m_spinAutoUpdateInterval->value()));
    }

    if (isChangeAllowed(m_ui.m_mcbOpenArticlesAutomatically)) {
      fd->setOpenArticlesDirectly(m_ui.m_cbOpenArticlesAutomatically->isChecked());
    }

    if (isChangeAllowed(m_ui.m_mcbFeedRtl)) {
      fd->setIsRtl(m_ui.m_cbFeedRTL->isChecked());
    }

    if (isChangeAllowed(m_ui.m_mcbAddAnyDateArticles)) {
      fd->setAddAnyDatetimeArticles(m_ui.m_cbAddAnyDateArticles->isChecked());
    }

    if (isChangeAllowed(m_ui.m_mcbAvoidOldArticles)) {
      fd->setDatetimeToAvoid(m_ui.m_gbAvoidOldArticles->isChecked() ? m_ui.m_dtDateTimeToAvoid->dateTime()
                                                                    : TextFactory::parseDateTime(0));
    }

    if (isChangeAllowed(m_ui.m_mcbDisableFeed)) {
      fd->setIsSwitchedOff(m_ui.m_cbDisableFeed->isChecked());
    }

    if (isChangeAllowed(m_ui.m_mcbSuppressFeed)) {
      fd->setIsQuiet(m_ui.m_cbSuppressFeed->isChecked());
    }

    if (!m_creatingNew) {
      // We need to make sure that common data are saved.
      QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

      DatabaseQueries::createOverwriteFeed(database, fd, m_serviceRoot->accountId(), fd->parent()->id());
    }
  }

  if (!m_creatingNew) {
    auto all_feeds = feeds<RootItem>();
    m_serviceRoot->itemChanged(all_feeds);
  }
}

bool FormFeedDetails::isChangeAllowed(MultiFeedEditCheckBox* mcb) const {
  return m_feeds.size() <= 1 || mcb->isChecked();
}

void FormFeedDetails::onAutoUpdateTypeChanged(int new_index) {
  Feed::AutoUpdateType auto_update_type =
    static_cast<Feed::AutoUpdateType>(m_ui.m_cmbAutoUpdateType->itemData(new_index).toInt());

  switch (auto_update_type) {
    case Feed::AutoUpdateType::DontAutoUpdate:
    case Feed::AutoUpdateType::DefaultAutoUpdate:
      m_ui.m_spinAutoUpdateInterval->setEnabled(false);
      break;

    default:
      m_ui.m_spinAutoUpdateInterval->setEnabled(true);
  }
}

void FormFeedDetails::createConnections() {
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormFeedDetails::acceptIfPossible);
  connect(m_ui.m_cmbAutoUpdateType,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &FormFeedDetails::onAutoUpdateTypeChanged);

  connect(m_ui.m_cbAddAnyDateArticles, &QCheckBox::toggled, this, [this](bool checked) {
    m_ui.m_gbAvoidOldArticles->setEnabled(!checked);
  });
}

void FormFeedDetails::loadFeedData() {
  Feed* fd = feed<Feed>();

  if (m_creatingNew) {
    GuiUtilities::applyDialogProperties(*this,
                                        qApp->icons()->fromTheme(QSL("application-rss+xml")),
                                        tr("Add new feed"));
  }
  else {
    if (m_feeds.size() == 1) {
      GuiUtilities::applyDialogProperties(*this, fd->fullIcon(), tr("Edit \"%1\"").arg(fd->title()));
    }
    else {
      GuiUtilities::applyDialogProperties(*this,
                                          qApp->icons()->fromTheme(QSL("application-rss+xml")),
                                          tr("Edit %n feeds", nullptr, m_feeds.size()));
    }
  }

  m_ui.m_cmbAutoUpdateType
    ->setCurrentIndex(m_ui.m_cmbAutoUpdateType->findData(QVariant::fromValue(int(fd->autoUpdateType()))));
  m_ui.m_spinAutoUpdateInterval->setValue(fd->autoUpdateInterval());
  m_ui.m_cbOpenArticlesAutomatically->setChecked(fd->openArticlesDirectly());
  m_ui.m_cbFeedRTL->setChecked(fd->isRtl());
  m_ui.m_cbAddAnyDateArticles->setChecked(fd->addAnyDatetimeArticles());
  m_ui.m_gbAvoidOldArticles->setChecked(fd->datetimeToAvoid().toMSecsSinceEpoch() > 0);
  m_ui.m_dtDateTimeToAvoid->setDateTime(fd->datetimeToAvoid());
  m_ui.m_cbDisableFeed->setChecked(fd->isSwitchedOff());
  m_ui.m_cbSuppressFeed->setChecked(fd->isQuiet());
}

void FormFeedDetails::acceptIfPossible() {
  try {
    apply();
    accept();
  }
  catch (const ApplicationException& ex) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot save feed properties"),
                          tr("Cannot save changes: %1").arg(ex.message()),
                          QSystemTrayIcon::MessageIcon::Critical},
                         {},
                         {},
                         this);
  }
}

void FormFeedDetails::initialize() {
  m_ui.setupUi(this);

  m_ui.m_dtDateTimeToAvoid
    ->setDisplayFormat(qApp->localization()->loadedLocale().dateTimeFormat(QLocale::FormatType::ShortFormat));

  // Setup auto-update options.
  m_ui.m_spinAutoUpdateInterval->setMode(TimeSpinBox::Mode::MinutesSeconds);
  m_ui.m_spinAutoUpdateInterval->setValue(DEFAULT_AUTO_UPDATE_INTERVAL);
  m_ui.m_cmbAutoUpdateType->addItem(tr("Fetch articles using global interval"),
                                    QVariant::fromValue(int(Feed::AutoUpdateType::DefaultAutoUpdate)));
  m_ui.m_cmbAutoUpdateType->addItem(tr("Fetch articles every"),
                                    QVariant::fromValue(int(Feed::AutoUpdateType::SpecificAutoUpdate)));
  m_ui.m_cmbAutoUpdateType->addItem(tr("Disable auto-fetching of articles"),
                                    QVariant::fromValue(int(Feed::AutoUpdateType::DontAutoUpdate)));
}
