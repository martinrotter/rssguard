// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/gui/standardfeeddetails.h"

#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/category.h"
#include "services/standard/standardfeed.h"

#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMimeData>
#include <QTextCodec>

StandardFeedDetails::StandardFeedDetails(QWidget* parent) : QWidget(parent) {
  ui.setupUi(this);

  ui.m_txtTitle->lineEdit()->setPlaceholderText(tr("Feed title"));
  ui.m_txtTitle->lineEdit()->setToolTip(tr("Set title for your feed."));
  ui.m_txtDescription->lineEdit()->setPlaceholderText(tr("Feed description"));
  ui.m_txtDescription->lineEdit()->setToolTip(tr("Set description for your feed."));
  ui.m_txtUrl->lineEdit()->setPlaceholderText(tr("Full feed url including scheme"));
  ui.m_txtUrl->lineEdit()->setToolTip(tr("Set url for your feed."));

  // Add standard feed types.
  ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Atom10), QVariant::fromValue(int(StandardFeed::Type::Atom10)));
  ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Rdf), QVariant::fromValue(int(StandardFeed::Type::Rdf)));
  ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Rss0X), QVariant::fromValue(int(StandardFeed::Type::Rss0X)));
  ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Rss2X), QVariant::fromValue(int(StandardFeed::Type::Rss2X)));
  ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Json), QVariant::fromValue(int(StandardFeed::Type::Json)));

  // Load available encodings.
  const QList<QByteArray> encodings = QTextCodec::availableCodecs();
  QStringList encoded_encodings;

  for (const QByteArray& encoding : encodings) {
    encoded_encodings.append(encoding);
  }

  // Sort encodings and add them.
  std::sort(encoded_encodings.begin(), encoded_encodings.end(), [](const QString& lhs, const QString& rhs) {
    return lhs.toLower() < rhs.toLower();
  });

  ui.m_cmbEncoding->addItems(encoded_encodings);

  // Setup menu & actions for icon selection.
  m_iconMenu = new QMenu(tr("Icon selection"), this);
  m_actionLoadIconFromFile = new QAction(qApp->icons()->fromTheme(QSL("image-x-generic")),
                                         tr("Load icon from file..."),
                                         this);
  m_actionUseDefaultIcon = new QAction(qApp->icons()->fromTheme(QSL("application-rss+xml")),
                                       tr("Use default icon from icon theme"),
                                       this);
  m_actionFetchIcon = new QAction(qApp->icons()->fromTheme(QSL("emblem-downloads")),
                                  tr("Fetch icon from feed"),
                                  this);
  m_iconMenu->addAction(m_actionFetchIcon);
  m_iconMenu->addAction(m_actionLoadIconFromFile);
  m_iconMenu->addAction(m_actionUseDefaultIcon);
  ui.m_btnIcon->setMenu(m_iconMenu);
  ui.m_txtUrl->lineEdit()->setFocus(Qt::TabFocusReason);

  // Set feed metadata fetch label.
  ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Information,
                                   tr("No metadata fetched so far."),
                                   tr("No metadata fetched so far."));

  connect(ui.m_txtTitle->lineEdit(), &BaseLineEdit::textChanged, this, &StandardFeedDetails::onTitleChanged);
  connect(ui.m_txtDescription->lineEdit(), &BaseLineEdit::textChanged, this, &StandardFeedDetails::onDescriptionChanged);
  connect(ui.m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &StandardFeedDetails::onUrlChanged);
  connect(m_actionLoadIconFromFile, &QAction::triggered, this, &StandardFeedDetails::onLoadIconFromFile);
  connect(m_actionUseDefaultIcon, &QAction::triggered, this, &StandardFeedDetails::onUseDefaultIcon);

  onTitleChanged(QString());
  onDescriptionChanged(QString());
  onUrlChanged(QString());
}

void StandardFeedDetails::guessIconOnly(const QString& url, const QString& username, const QString& password) {
  QPair<StandardFeed*, QNetworkReply::NetworkError> result = StandardFeed::guessFeed(url, username, password);

  if (result.first != nullptr) {
    // Icon or whole feed was guessed.
    ui.m_btnIcon->setIcon(result.first->icon());

    if (result.second == QNetworkReply::NoError) {
      ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Ok,
                                       tr("Icon fetched successfully."),
                                       tr("Icon metadata fetched."));
    }
    else {
      ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Warning,
                                       tr("Result: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                       tr("Icon metadata not fetched."));
    }

    // Remove temporary feed object.
    delete result.first;
  }
  else {
    // No feed guessed, even no icon available.
    ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                     tr("Error: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                     tr("No icon fetched."));
  }
}

void StandardFeedDetails::guessFeed(const QString& url, const QString& username, const QString& password) {
  QPair<StandardFeed*, QNetworkReply::NetworkError> result = StandardFeed::guessFeed(url, username, password);

  if (result.first != nullptr) {
    // Icon or whole feed was guessed.
    ui.m_btnIcon->setIcon(result.first->icon());
    ui.m_txtTitle->lineEdit()->setText(result.first->title());
    ui.m_txtDescription->lineEdit()->setText(result.first->description());
    ui.m_cmbType->setCurrentIndex(ui.m_cmbType->findData(QVariant::fromValue((int) result.first->type())));
    int encoding_index = ui.m_cmbEncoding->findText(result.first->encoding(), Qt::MatchFixedString);

    if (encoding_index >= 0) {
      ui.m_cmbEncoding->setCurrentIndex(encoding_index);
    }
    else {
      ui.m_cmbEncoding->setCurrentIndex(ui.m_cmbEncoding->findText(DEFAULT_FEED_ENCODING, Qt::MatchFixedString));
    }

    if (result.second == QNetworkReply::NoError) {
      ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Ok,
                                       tr("All metadata fetched successfully."),
                                       tr("Feed and icon metadata fetched."));
    }
    else {
      ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Warning,
                                       tr("Result: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                       tr("Feed or icon metadata not fetched."));
    }

    // Remove temporary feed object.
    delete result.first;
  }
  else {
    // No feed guessed, even no icon available.
    ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                     tr("Error: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                     tr("No metadata fetched."));
  }
}

void StandardFeedDetails::onTitleChanged(const QString& new_title) {
  if (new_title.simplified().size() >= MIN_CATEGORY_NAME_LENGTH) {
    ui.m_txtTitle->setStatus(LineEditWithStatus::StatusType::Ok, tr("Feed name is ok."));
  }
  else {
    ui.m_txtTitle->setStatus(LineEditWithStatus::StatusType::Error, tr("Feed name is too short."));
  }
}

void StandardFeedDetails::onDescriptionChanged(const QString& new_description) {
  if (new_description.simplified().isEmpty()) {
    ui.m_txtDescription->setStatus(LineEditWithStatus::StatusType::Warning, tr("Description is empty."));
  }
  else {
    ui.m_txtDescription->setStatus(LineEditWithStatus::StatusType::Ok, tr("The description is ok."));
  }
}

void StandardFeedDetails::onUrlChanged(const QString& new_url) {
  if (QRegularExpression(URL_REGEXP).match(new_url).hasMatch()) {
    // New url is well-formed.
    ui.m_txtUrl->setStatus(LineEditWithStatus::StatusType::Ok, tr("The URL is ok."));
  }
  else if (!new_url.simplified().isEmpty()) {
    // New url is not well-formed but is not empty on the other hand.
    ui.m_txtUrl->setStatus(LineEditWithStatus::StatusType::Warning,
                           tr(R"(The URL does not meet standard pattern. Does your URL start with "http://" or "https://" prefix.)"));
  }
  else {
    // New url is empty.
    ui.m_txtUrl->setStatus(LineEditWithStatus::StatusType::Error, tr("The URL is empty."));
  }
}

void StandardFeedDetails::onLoadIconFromFile() {
  QFileDialog dialog(this, tr("Select icon file for the feed"),
                     qApp->homeFolder(), tr("Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)"));

  dialog.setFileMode(QFileDialog::FileMode::ExistingFile);
  dialog.setWindowIcon(qApp->icons()->fromTheme(QSL("image-x-generic")));
  dialog.setOptions(QFileDialog::Option::DontUseNativeDialog | QFileDialog::Option::ReadOnly);
  dialog.setViewMode(QFileDialog::ViewMode::Detail);
  dialog.setLabelText(QFileDialog::DialogLabel::Accept, tr("Select icon"));
  dialog.setLabelText(QFileDialog::DialogLabel::Reject, tr("Cancel"));

  //: Label for field with icon file name textbox for selection dialog.
  dialog.setLabelText(QFileDialog::DialogLabel::LookIn, tr("Look in:"));
  dialog.setLabelText(QFileDialog::DialogLabel::FileName, tr("Icon name:"));
  dialog.setLabelText(QFileDialog::DialogLabel::FileType, tr("Icon type:"));

  if (dialog.exec() == QDialog::DialogCode::Accepted) {
    ui.m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void StandardFeedDetails::onUseDefaultIcon() {
  ui.m_btnIcon->setIcon(QIcon());
}

void StandardFeedDetails::prepareForNewFeed(RootItem* parent_to_select, const QString& url) {
  // Make sure that "default" icon is used as the default option for new
  // feed.
  m_actionUseDefaultIcon->trigger();
  int default_encoding_index = ui.m_cmbEncoding->findText(DEFAULT_FEED_ENCODING);

  if (default_encoding_index >= 0) {
    ui.m_cmbEncoding->setCurrentIndex(default_encoding_index);
  }

  if (parent_to_select != nullptr) {
    if (parent_to_select->kind() == RootItem::Kind::Category) {
      ui.m_cmbParentCategory->setCurrentIndex(ui.m_cmbParentCategory->findData(QVariant::fromValue((void*)parent_to_select)));
    }
    else if (parent_to_select->kind() == RootItem::Kind::Feed) {
      int target_item = ui.m_cmbParentCategory->findData(QVariant::fromValue((void*)parent_to_select->parent()));

      if (target_item >= 0) {
        ui.m_cmbParentCategory->setCurrentIndex(target_item);
      }
    }
  }

  if (!url.isEmpty()) {
    ui.m_txtUrl->lineEdit()->setText(url);
  }
  else if (Application::clipboard()->mimeData()->hasText()) {
    ui.m_txtUrl->lineEdit()->setText(Application::clipboard()->text());
  }

  ui.m_txtUrl->setFocus();
}

void StandardFeedDetails::setExistingFeed(StandardFeed* feed) {
  ui.m_cmbParentCategory->setCurrentIndex(ui.m_cmbParentCategory->findData(QVariant::fromValue((void*)feed->parent())));
  ui.m_txtTitle->lineEdit()->setText(feed->title());
  ui.m_txtDescription->lineEdit()->setText(feed->description());
  ui.m_btnIcon->setIcon(feed->icon());
  ui.m_txtUrl->lineEdit()->setText(feed->url());
  ui.m_cmbType->setCurrentIndex(ui.m_cmbType->findData(QVariant::fromValue(int(feed->type()))));
  ui.m_cmbEncoding->setCurrentIndex(ui.m_cmbEncoding->findData(feed->encoding(),
                                                               Qt::ItemDataRole::DisplayRole,
                                                               Qt::MatchFlag::MatchFixedString));
}

void StandardFeedDetails::loadCategories(const QList<Category*>& categories, RootItem* root_item) {
  ui.m_cmbParentCategory->addItem(root_item->fullIcon(), root_item->title(), QVariant::fromValue((void*) root_item));

  for (Category* category : categories) {
    ui.m_cmbParentCategory->addItem(category->fullIcon(), category->title(), QVariant::fromValue((void*) category));
  }
}
