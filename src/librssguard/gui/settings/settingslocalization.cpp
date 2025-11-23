// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingslocalization.h"

#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/localization.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

SettingsLocalization::SettingsLocalization(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(nullptr),
    m_urlPercentages(QUrl(QSL("https://api.crowdin.com/api/v2/projects/608575/languages/progress?limit=100"))),
    m_urlPeople(QUrl(QSL("https://api.crowdin.com/api/v2/projects/608575/members?limit=500"))) {}

SettingsLocalization::~SettingsLocalization() {
  if (m_ui != nullptr) {
    delete m_ui;
  }
}

void SettingsLocalization::loadUi() {
  m_ui = new Ui::SettingsLocalization();

  m_ui->setupUi(this);
  m_ui->m_lblAuthors->label()->setWordWrap(true);
  m_ui->m_treeLanguages->setColumnCount(3);
  m_ui->m_treeLanguages->setHeaderHidden(false);
  m_ui->m_treeLanguages->setHeaderLabels(QStringList() << tr("Language") << tr("Code") << tr("Translation progress"));

  m_ui->m_lblHelp->setText(tr(R"(Help us to improve %1 <a href="%2">translations</a>.)")
                             .arg(QSL(APP_NAME), QSL("https://crowdin.com/project/rssguard")));

  connect(m_ui->m_lblHelp,
          &QLabel::linkActivated,
          qApp->web(),
          QOverload<const QUrl&>::of(&WebFactory::openUrlInExternalBrowser));

  // Setup languages.
  m_ui->m_treeLanguages->header()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);

  connect(m_ui->m_treeLanguages, &QTreeWidget::currentItemChanged, this, &SettingsLocalization::requireRestart);
  connect(m_ui->m_treeLanguages, &QTreeWidget::currentItemChanged, this, &SettingsLocalization::dirtifySettings);

  SettingsPanel::loadUi();
}

QIcon SettingsLocalization::icon() const {
  return qApp->icons()->fromTheme(QSL("text-x-gettext-translation"));
}

void SettingsLocalization::langMetadataDownloaded(const QUrl& url,
                                                  QNetworkReply::NetworkError status,
                                                  int http_code,
                                                  QByteArray contents) {
  auto* down = qobject_cast<Downloader*>(sender());

  if (url == m_urlPercentages) {
    if (status == QNetworkReply::NetworkError::NoError) {
      m_dataPercentages = contents;
      down->downloadFile(m_urlPeople.toString(),
                         qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt());
    }
    else {
      qCriticalNN << LOGSEC_NETWORK << "Failed to download languages metadata.";

      if (down != nullptr) {
        down->deleteLater();
      }
    }
  }
  else {
    if (status == QNetworkReply::NetworkError::NoError) {
      m_dataPeople = contents;

      QMap<QString, int> percentages_langs;
      QString all_translators;

      QJsonDocument stats_doc = QJsonDocument::fromJson(m_dataPercentages);
      QJsonDocument people_doc = QJsonDocument::fromJson(m_dataPeople);
      QJsonArray people_arr = people_doc.object()["data"].toArray();
      std::vector<QString> people_desc;

      std::transform(people_arr.begin(), people_arr.end(), std::back_inserter(people_desc), [](const QJsonValue& b) {
        return b.toObject()["data"].toObject()["username"].toString();
      });

      all_translators = std::accumulate(std::next(people_desc.begin()),
                                        people_desc.end(),
                                        people_desc.at(0),
                                        [](const QString& lhs, const QString& rhs) {
                                          return QString(lhs + ", " + rhs);
                                        });

      for (const QJsonValue& val_lang : stats_doc.object()["data"].toArray()) {
        QString lang_id = val_lang.toObject()["data"].toObject()["languageId"].toString().replace(QSL("-"), QSL("_"));
        int lang_completion = val_lang.toObject()["data"].toObject()["translationProgress"].toInt();

        if (lang_id == QSL("es_ES")) {
          lang_id = QSL("es");
        }

        if (lang_id == QSL("en_US")) {
          lang_completion = 100;
        }

        percentages_langs.insert(lang_id, lang_completion);
      }

      if (all_translators.isEmpty()) {
        m_ui->m_lblAuthors->setStatus(WidgetWithStatus::StatusType::Information,
                                      tr("Big thanks to all translators!"),
                                      tr("Big thanks to all translators!"));
      }
      else {
        m_ui->m_lblAuthors->setStatus(WidgetWithStatus::StatusType::Information,
                                      tr("Translations provided by: %1").arg(all_translators),
                                      tr("Big thanks to all translators!"));
      }

      for (int i = 0; i < m_ui->m_treeLanguages->topLevelItemCount(); i++) {
        auto* it = m_ui->m_treeLanguages->topLevelItem(i);
        int perc_translated = percentages_langs.value(it->text(1));
        QColor col_translated = QColor::fromHsv(perc_translated, 200, 230);

        it->setText(2, QSL("%1 %").arg(perc_translated > 0 ? QString::number(perc_translated) : QSL("?")));
        it->setIcon(2, IconFactory::generateIcon(col_translated));
      }
    }
    else {
      qCriticalNN << LOGSEC_NETWORK << "Failed to download languages people metadata.";
    }

    if (down != nullptr) {
      down->deleteLater();
    }
  }
}

void SettingsLocalization::loadSettings() {
  onBeginLoadSettings();

  auto langs = qApp->localization()->installedLanguages();
  auto* downl = new Downloader(this);

  // Also, load statistics with restricted access token.
  QList<QPair<QByteArray, QByteArray>> hdrs = {
    {"Authorization",
     "Bearer "
     "0fbcad4c39d21a55f63f8a1b6d07cc56bb1e2eb2047bfaf1ee22425e3edf1c2b217f4d13b3cebba9"}};

  downl->appendRawHeaders(hdrs);
  connect(downl,
          &Downloader::completed,
          this,
          &SettingsLocalization::langMetadataDownloaded,
          Qt::ConnectionType::QueuedConnection);
  downl->downloadFile(m_urlPercentages.toString(),
                      qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt());

  for (const Language& language : std::as_const(langs)) {
    auto* item = new QTreeWidgetItem(m_ui->m_treeLanguages);

    item->setText(0, language.m_name);
    item->setText(1, language.m_code);
    item->setText(2, QSL("? %"));

    item->setIcon(0, qApp->icons()->miscIcon(QSL(FLAG_ICON_SUBFOLDER) + QDir::separator() + language.m_code));
    item->setIcon(2, IconFactory::generateIcon(Qt::GlobalColor::blue));
  }

  m_ui->m_treeLanguages->sortByColumn(0, Qt::SortOrder::AscendingOrder);

  QList<QTreeWidgetItem*> matching_items =
    m_ui->m_treeLanguages->findItems(qApp->localization()->loadedLanguage(), Qt::MatchFlag::MatchContains, 1);

  if (!matching_items.isEmpty()) {
    m_ui->m_treeLanguages->setCurrentItem(matching_items[0]);
  }

  onEndLoadSettings();
}

void SettingsLocalization::saveSettings() {
  onBeginSaveSettings();

  if (m_ui->m_treeLanguages->currentItem() == nullptr) {
    qWarningNN << LOGSEC_GUI << "No localizations loaded in settings dialog, so no saving for them.";
    return;
  }

  const QString actual_lang = qApp->localization()->loadedLanguage();
  const QString new_lang = m_ui->m_treeLanguages->currentItem()->text(1);

  // Save prompt for restart if language has changed.
  if (new_lang != actual_lang) {
    int perc = m_ui->m_treeLanguages->currentItem()->toolTip(2).toInt();

    if (perc > 0 && perc < 75) {
      const QMessageBox::StandardButton clicked_button = MsgBox::
        show(this,
             QMessageBox::Icon::Question,
             tr("Translators needed!"),
             tr("The translation '%1' is incomplete and anyone able to help with translating %2 is greatly welcomed.")
               .arg(new_lang, QSL(APP_NAME)),
             tr("Do you want to help with the translation now?"),
             {},
             QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
             QMessageBox::StandardButton::Yes);

      if (clicked_button == QMessageBox::StandardButton::Yes) {
        qApp->web()->openUrlInExternalBrowser(QSL("https://crowdin.com/project/rssguard"));
      }
    }

    requireRestart();
    settings()->setValue(GROUP(General), General::Language, new_lang);
  }

  onEndSaveSettings();
}
