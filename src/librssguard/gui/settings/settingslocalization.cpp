// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingslocalization.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/localization.h"
#include "miscellaneous/settings.h"

SettingsLocalization::SettingsLocalization(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsLocalization) {
  m_ui->setupUi(this);
  m_ui->m_treeLanguages->setColumnCount(3);
  m_ui->m_treeLanguages->setHeaderHidden(false);
  m_ui->m_treeLanguages->setHeaderLabels(QStringList()
                                         << /*: Language column of language list. */ tr("Language")
                                         << /*: Lang. code column of language list. */ tr("Code")
                                         << tr("Author"));

  // Setup languages.
  m_ui->m_treeLanguages->header()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);
  connect(m_ui->m_treeLanguages, &QTreeWidget::currentItemChanged, this, &SettingsLocalization::requireRestart);
  connect(m_ui->m_treeLanguages, &QTreeWidget::currentItemChanged, this, &SettingsLocalization::dirtifySettings);
}

SettingsLocalization::~SettingsLocalization() {
  delete m_ui;
}

void SettingsLocalization::loadSettings() {
  onBeginLoadSettings();

  auto langs = qApp->localization()->installedLanguages();

  for (const Language& language : qAsConst(langs)) {
    auto* item = new QTreeWidgetItem(m_ui->m_treeLanguages);

    item->setText(0, language.m_name);
    item->setText(1, language.m_code);
    item->setText(2, language.m_author);
    item->setIcon(0, qApp->icons()->miscIcon(QString(FLAG_ICON_SUBFOLDER) + QDir::separator() + language.m_code));
  }

  m_ui->m_treeLanguages->sortByColumn(0, Qt::SortOrder::AscendingOrder);
  QList<QTreeWidgetItem*> matching_items = m_ui->m_treeLanguages->findItems(qApp->localization()->loadedLanguage(),
                                                                            Qt::MatchFlag::MatchContains,
                                                                            1);

  if (!matching_items.isEmpty()) {
    m_ui->m_treeLanguages->setCurrentItem(matching_items[0]);
  }

  onEndLoadSettings();
}

void SettingsLocalization::saveSettings() {
  onBeginSaveSettings();

  if (m_ui->m_treeLanguages->currentItem() == nullptr) {
    qDebug("No localizations loaded in settings dialog, so no saving for them.");
    return;
  }

  const QString actual_lang = qApp->localization()->loadedLanguage();
  const QString new_lang = m_ui->m_treeLanguages->currentItem()->text(1);

  // Save prompt for restart if language has changed.
  if (new_lang != actual_lang) {
    requireRestart();
    settings()->setValue(GROUP(General), General::Language, new_lang);
  }

  onEndSaveSettings();
}
