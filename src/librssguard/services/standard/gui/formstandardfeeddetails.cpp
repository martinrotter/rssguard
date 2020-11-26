// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/gui/formstandardfeeddetails.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/serviceroot.h"
#include "services/standard/gui/standardfeeddetails.h"
#include "services/standard/standardfeed.h"

#include <QClipboard>
#include <QFileDialog>
#include <QMimeData>
#include <QTextCodec>

FormStandardFeedDetails::FormStandardFeedDetails(ServiceRoot* service_root, QWidget* parent)
  : FormFeedDetails(service_root, parent), m_standardFeedDetails(new StandardFeedDetails(this)) {
  insertCustomTab(m_standardFeedDetails, tr("General"), 0);
  activateTab(0);

  m_standardFeedDetails->ui->m_txtTitle->lineEdit()->setPlaceholderText(tr("Feed title"));
  m_standardFeedDetails->ui->m_txtTitle->lineEdit()->setToolTip(tr("Set title for your feed."));
  m_standardFeedDetails->ui->m_txtDescription->lineEdit()->setPlaceholderText(tr("Feed description"));
  m_standardFeedDetails->ui->m_txtDescription->lineEdit()->setToolTip(tr("Set description for your feed."));
  m_standardFeedDetails->ui->m_txtUrl->lineEdit()->setPlaceholderText(tr("Full feed url including scheme"));
  m_standardFeedDetails->ui->m_txtUrl->lineEdit()->setToolTip(tr("Set url for your feed."));

  // Add standard feed types.
  m_standardFeedDetails->ui->m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Atom10), QVariant::fromValue(int(StandardFeed::Type::Atom10)));
  m_standardFeedDetails->ui->m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Rdf), QVariant::fromValue(int(StandardFeed::Type::Rdf)));
  m_standardFeedDetails->ui->m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Rss0X), QVariant::fromValue(int(StandardFeed::Type::Rss0X)));
  m_standardFeedDetails->ui->m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Rss2X), QVariant::fromValue(int(StandardFeed::Type::Rss2X)));
  m_standardFeedDetails->ui->m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Json), QVariant::fromValue(int(StandardFeed::Type::Json)));

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

  m_standardFeedDetails->ui->m_cmbEncoding->addItems(encoded_encodings);

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
  m_standardFeedDetails->ui->m_btnIcon->setMenu(m_iconMenu);
  m_standardFeedDetails->ui->m_txtUrl->lineEdit()->setFocus(Qt::TabFocusReason);

  // Set feed metadata fetch label.
  m_standardFeedDetails->ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Information,
                                                           tr("No metadata fetched so far."),
                                                           tr("No metadata fetched so far."));

  connect(m_standardFeedDetails->ui->m_txtTitle->lineEdit(), &BaseLineEdit::textChanged, this, &FormStandardFeedDetails::onTitleChanged);
  connect(m_standardFeedDetails->ui->m_txtDescription->lineEdit(), &BaseLineEdit::textChanged, this, &FormStandardFeedDetails::onDescriptionChanged);
  connect(m_standardFeedDetails->ui->m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormStandardFeedDetails::onUrlChanged);
  connect(m_standardFeedDetails->ui->m_btnFetchMetadata, &QPushButton::clicked, this, &FormStandardFeedDetails::guessFeed);
  connect(m_actionFetchIcon, &QAction::triggered, this, &FormStandardFeedDetails::guessIconOnly);
  connect(m_actionLoadIconFromFile, &QAction::triggered, this, &FormStandardFeedDetails::onLoadIconFromFile);
  connect(m_actionUseDefaultIcon, &QAction::triggered, this, &FormStandardFeedDetails::onUseDefaultIcon);

  onTitleChanged(QString());
  onDescriptionChanged(QString());
  onUrlChanged(QString());
}

void FormStandardFeedDetails::loadCategories(const QList<Category*>& categories, RootItem* root_item) {
  m_standardFeedDetails->ui->m_cmbParentCategory->addItem(root_item->fullIcon(),
                                                          root_item->title(),
                                                          QVariant::fromValue((void*) root_item));

  for (Category* category : categories) {
    m_standardFeedDetails->ui->m_cmbParentCategory->addItem(category->fullIcon(),
                                                            category->title(),
                                                            QVariant::fromValue((void*) category));
  }
}

int FormStandardFeedDetails::addEditFeed(Feed* input_feed, RootItem* parent_to_select, const QString& url) {
  // Load categories.
  loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);

  if (input_feed == nullptr) {
    // User is adding new feed.
    setWindowTitle(tr("Add new feed"));

    // Make sure that "default" icon is used as the default option for new
    // feed.
    m_actionUseDefaultIcon->trigger();
    int default_encoding_index = m_standardFeedDetails->ui->m_cmbEncoding->findText(DEFAULT_FEED_ENCODING);

    if (default_encoding_index >= 0) {
      m_standardFeedDetails->ui->m_cmbEncoding->setCurrentIndex(default_encoding_index);
    }

    if (parent_to_select != nullptr) {
      if (parent_to_select->kind() == RootItem::Kind::Category) {
        m_standardFeedDetails->ui->m_cmbParentCategory->setCurrentIndex(m_standardFeedDetails->ui->m_cmbParentCategory->findData(QVariant::fromValue((void*)
                                                                                                                                                     parent_to_select)));
      }
      else if (parent_to_select->kind() == RootItem::Kind::Feed) {
        int target_item = m_standardFeedDetails->ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) parent_to_select->parent()));

        if (target_item >= 0) {
          m_standardFeedDetails->ui->m_cmbParentCategory->setCurrentIndex(target_item);
        }
      }
    }

    if (!url.isEmpty()) {
      m_standardFeedDetails->ui->m_txtUrl->lineEdit()->setText(url);
    }
    else if (Application::clipboard()->mimeData()->hasText()) {
      m_standardFeedDetails->ui->m_txtUrl->lineEdit()->setText(Application::clipboard()->text());
    }
  }
  else {
    // User is editing existing category.
    setWindowTitle(tr("Edit feed '%1'").arg(input_feed->title()));
    setEditableFeed(input_feed);
  }

  // Run the dialog.
  return QDialog::exec();
}

void FormStandardFeedDetails::onTitleChanged(const QString& new_title) {
  if (new_title.simplified().size() >= MIN_CATEGORY_NAME_LENGTH) {
    m_standardFeedDetails->ui->m_txtTitle->setStatus(LineEditWithStatus::StatusType::Ok, tr("Feed name is ok."));
  }
  else {
    m_standardFeedDetails->ui->m_txtTitle->setStatus(LineEditWithStatus::StatusType::Error, tr("Feed name is too short."));
  }
}

void FormStandardFeedDetails::onDescriptionChanged(const QString& new_description) {
  if (new_description.simplified().isEmpty()) {
    m_standardFeedDetails->ui->m_txtDescription->setStatus(LineEditWithStatus::StatusType::Warning, tr("Description is empty."));
  }
  else {
    m_standardFeedDetails->ui->m_txtDescription->setStatus(LineEditWithStatus::StatusType::Ok, tr("The description is ok."));
  }
}

void FormStandardFeedDetails::onUrlChanged(const QString& new_url) {
  if (QRegularExpression(URL_REGEXP).match(new_url).hasMatch()) {
    // New url is well-formed.
    m_standardFeedDetails->ui->m_txtUrl->setStatus(LineEditWithStatus::StatusType::Ok, tr("The URL is ok."));
  }
  else if (!new_url.simplified().isEmpty()) {
    // New url is not well-formed but is not empty on the other hand.
    m_standardFeedDetails->ui->m_txtUrl->setStatus(LineEditWithStatus::StatusType::Warning,
                                                   tr(R"(The URL does not meet standard pattern. Does your URL start with "http://" or "https://" prefix.)"));
  }
  else {
    // New url is empty.
    m_standardFeedDetails->ui->m_txtUrl->setStatus(LineEditWithStatus::StatusType::Error, tr("The URL is empty."));
  }
}

void FormStandardFeedDetails::onLoadIconFromFile() {
  QFileDialog dialog(this, tr("Select icon file for the feed"),
                     qApp->homeFolder(), tr("Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)"));

  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setWindowIcon(qApp->icons()->fromTheme(QSL("image-x-generic")));
  dialog.setOptions(QFileDialog::DontUseNativeDialog | QFileDialog::ReadOnly);
  dialog.setViewMode(QFileDialog::Detail);
  dialog.setLabelText(QFileDialog::Accept, tr("Select icon"));
  dialog.setLabelText(QFileDialog::Reject, tr("Cancel"));

  //: Label for field with icon file name textbox for selection dialog.
  dialog.setLabelText(QFileDialog::LookIn, tr("Look in:"));
  dialog.setLabelText(QFileDialog::FileName, tr("Icon name:"));
  dialog.setLabelText(QFileDialog::FileType, tr("Icon type:"));

  if (dialog.exec() == QDialog::Accepted) {
    m_standardFeedDetails->ui->m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void FormStandardFeedDetails::onUseDefaultIcon() {
  m_standardFeedDetails->ui->m_btnIcon->setIcon(QIcon());
}

void FormStandardFeedDetails::guessFeed() {
  QPair<StandardFeed*, QNetworkReply::NetworkError> result = StandardFeed::guessFeed(m_standardFeedDetails->ui->m_txtUrl->lineEdit()->text(),
                                                                                     m_ui->m_txtUsername->lineEdit()->text(),
                                                                                     m_ui->m_txtPassword->lineEdit()->text());

  if (result.first != nullptr) {
    // Icon or whole feed was guessed.
    m_standardFeedDetails->ui->m_btnIcon->setIcon(result.first->icon());
    m_standardFeedDetails->ui->m_txtTitle->lineEdit()->setText(result.first->title());
    m_standardFeedDetails->ui->m_txtDescription->lineEdit()->setText(result.first->description());
    m_standardFeedDetails->ui->m_cmbType->setCurrentIndex(m_standardFeedDetails->ui->m_cmbType->findData(QVariant::fromValue((int) result.first->type())));
    int encoding_index = m_standardFeedDetails->ui->m_cmbEncoding->findText(result.first->encoding(), Qt::MatchFixedString);

    if (encoding_index >= 0) {
      m_standardFeedDetails->ui->m_cmbEncoding->setCurrentIndex(encoding_index);
    }
    else {
      m_standardFeedDetails->ui->m_cmbEncoding->setCurrentIndex(m_standardFeedDetails->ui->m_cmbEncoding->findText(DEFAULT_FEED_ENCODING,
                                                                                                                   Qt::MatchFixedString));
    }

    if (result.second == QNetworkReply::NoError) {
      m_standardFeedDetails->ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Ok,
                                                               tr("All metadata fetched successfully."),
                                                               tr("Feed and icon metadata fetched."));
    }
    else {
      m_standardFeedDetails->ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Warning,
                                                               tr("Result: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                                               tr("Feed or icon metadata not fetched."));
    }

    // Remove temporary feed object.
    delete result.first;
  }
  else {
    // No feed guessed, even no icon available.
    m_standardFeedDetails->ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                                             tr("Error: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                                             tr("No metadata fetched."));
  }
}

void FormStandardFeedDetails::guessIconOnly() {
  QPair<StandardFeed*, QNetworkReply::NetworkError> result = StandardFeed::guessFeed(m_standardFeedDetails->ui->m_txtUrl->lineEdit()->text(),
                                                                                     m_ui->m_txtUsername->lineEdit()->text(),
                                                                                     m_ui->m_txtPassword->lineEdit()->text());

  if (result.first != nullptr) {
    // Icon or whole feed was guessed.
    m_standardFeedDetails->ui->m_btnIcon->setIcon(result.first->icon());

    if (result.second == QNetworkReply::NoError) {
      m_standardFeedDetails->ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Ok,
                                                               tr("Icon fetched successfully."),
                                                               tr("Icon metadata fetched."));
    }
    else {
      m_standardFeedDetails->ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Warning,
                                                               tr("Result: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                                               tr("Icon metadata not fetched."));
    }

    // Remove temporary feed object.
    delete result.first;
  }
  else {
    // No feed guessed, even no icon available.
    m_standardFeedDetails->ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                                             tr("Error: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                                             tr("No icon fetched."));
  }
}

void FormStandardFeedDetails::apply() {
  RootItem* parent =
    static_cast<RootItem*>(m_standardFeedDetails->ui->m_cmbParentCategory->itemData(
                             m_standardFeedDetails->ui->m_cmbParentCategory->currentIndex()).value<void*>());

  StandardFeed::Type type =
    static_cast<StandardFeed::Type>(m_standardFeedDetails->ui->m_cmbType->itemData(m_standardFeedDetails->ui->m_cmbType->currentIndex()).value<int>());
  auto* new_feed = new StandardFeed();

  // Setup data for new_feed.
  new_feed->setTitle(m_standardFeedDetails->ui->m_txtTitle->lineEdit()->text());
  new_feed->setCreationDate(QDateTime::currentDateTime());
  new_feed->setDescription(m_standardFeedDetails->ui->m_txtDescription->lineEdit()->text());
  new_feed->setIcon(m_standardFeedDetails->ui->m_btnIcon->icon());
  new_feed->setEncoding(m_standardFeedDetails->ui->m_cmbEncoding->currentText());
  new_feed->setType(type);
  new_feed->setUrl(m_standardFeedDetails->ui->m_txtUrl->lineEdit()->text());
  new_feed->setPasswordProtected(m_ui->m_gbAuthentication->isChecked());
  new_feed->setUsername(m_ui->m_txtUsername->lineEdit()->text());
  new_feed->setPassword(m_ui->m_txtPassword->lineEdit()->text());
  new_feed->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(
                                                                  m_ui->m_cmbAutoUpdateType->currentIndex()).toInt()));
  new_feed->setAutoUpdateInitialInterval(int(m_ui->m_spinAutoUpdateInterval->value()));

  if (m_editableFeed == nullptr) {
    // Add the feed.
    if (new_feed->addItself(parent)) {
      m_serviceRoot->requestItemReassignment(new_feed, parent);
      accept();
    }
    else {
      delete new_feed;
      qApp->showGuiMessage(tr("Cannot add feed"),
                           tr("Feed was not added due to error."),
                           QSystemTrayIcon::Critical, this, true);
    }
  }
  else {
    new_feed->setParent(parent);

    // Edit the feed.
    bool edited = qobject_cast<StandardFeed*>(m_editableFeed)->editItself(new_feed);

    if (edited) {
      m_serviceRoot->requestItemReassignment(m_editableFeed, new_feed->parent());
      accept();
    }
    else {
      qApp->showGuiMessage(tr("Cannot edit feed"),
                           tr("Feed was not edited due to error."),
                           QSystemTrayIcon::Critical, this, true);
    }

    delete new_feed;
  }
}

void FormStandardFeedDetails::setEditableFeed(Feed* editable_feed) {
  FormFeedDetails::setEditableFeed(editable_feed);

  auto* feed = qobject_cast<StandardFeed*>(editable_feed);

  m_standardFeedDetails->ui->m_cmbParentCategory->setCurrentIndex(m_standardFeedDetails->ui->m_cmbParentCategory->findData(QVariant::fromValue((void*)
                                                                                                                                               editable_feed->
                                                                                                                                               parent())));
  m_standardFeedDetails->ui->m_txtTitle->lineEdit()->setText(editable_feed->title());
  m_standardFeedDetails->ui->m_txtDescription->lineEdit()->setText(editable_feed->description());
  m_standardFeedDetails->ui->m_btnIcon->setIcon(editable_feed->icon());
  m_standardFeedDetails->ui->m_txtUrl->lineEdit()->setText(editable_feed->url());
  m_standardFeedDetails->ui->m_cmbType->setCurrentIndex(m_standardFeedDetails->ui->m_cmbType->findData(QVariant::fromValue(int(feed->type()))));
  m_standardFeedDetails->ui->m_cmbEncoding->setCurrentIndex(m_standardFeedDetails->ui->m_cmbEncoding->findData(feed->encoding(), Qt::DisplayRole,
                                                                                                               Qt::MatchFixedString));
}
