// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formfeeddetails.h"

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "gui/baselineedit.h"
#include "gui/messagebox.h"
#include "gui/systemtrayicon.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/rootitem.h"
#include "services/standard/standardfeed.h"
#include "services/standard/standardserviceroot.h"

#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMimeData>
#include <QNetworkReply>
#include <QPair>
#include <QPushButton>
#include <QTextCodec>

FormFeedDetails::FormFeedDetails(ServiceRoot* service_root, QWidget* parent)
  : QDialog(parent),
  m_editableFeed(nullptr),
  m_serviceRoot(service_root) {
  initialize();
  createConnections();

  // Initialize that shit.
  onTitleChanged(QString());
  onDescriptionChanged(QString());
  onUrlChanged(QString());
  onUsernameChanged(QString());
  onPasswordChanged(QString());
}

FormFeedDetails::~FormFeedDetails() = default;

int FormFeedDetails::addEditFeed(Feed* input_feed, RootItem* parent_to_select, const QString& url) {
  // Load categories.
  loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);

  if (input_feed == nullptr) {
    // User is adding new feed.
    setWindowTitle(tr("Add new feed"));

    // Make sure that "default" icon is used as the default option for new
    // feed.
    m_actionUseDefaultIcon->trigger();
    int default_encoding_index = m_ui->m_cmbEncoding->findText(DEFAULT_FEED_ENCODING);

    if (default_encoding_index >= 0) {
      m_ui->m_cmbEncoding->setCurrentIndex(default_encoding_index);
    }

    if (parent_to_select != nullptr) {
      if (parent_to_select->kind() == RootItemKind::Category) {
        m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) parent_to_select)));
      }
      else if (parent_to_select->kind() == RootItemKind::Feed) {
        int target_item = m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) parent_to_select->parent()));

        if (target_item >= 0) {
          m_ui->m_cmbParentCategory->setCurrentIndex(target_item);
        }
      }
    }

    if (!url.isEmpty()) {
      m_ui->m_txtUrl->lineEdit()->setText(url);
    }
    else if (Application::clipboard()->mimeData()->hasText()) {
      m_ui->m_txtUrl->lineEdit()->setText(Application::clipboard()->text());
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

void FormFeedDetails::onTitleChanged(const QString& new_title) {
  if (new_title.simplified().size() >= MIN_CATEGORY_NAME_LENGTH) {
    m_ui->m_txtTitle->setStatus(LineEditWithStatus::Ok, tr("Feed name is ok."));
  }
  else {
    m_ui->m_txtTitle->setStatus(LineEditWithStatus::Error, tr("Feed name is too short."));
  }
}

void FormFeedDetails::onDescriptionChanged(const QString& new_description) {
  if (new_description.simplified().isEmpty()) {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::Warning, tr("Description is empty."));
  }
  else {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::Ok, tr("The description is ok."));
  }
}

void FormFeedDetails::onUrlChanged(const QString& new_url) {
  if (QRegularExpression(URL_REGEXP).match(new_url).hasMatch()) {
    // New url is well-formed.
    m_ui->m_txtUrl->setStatus(LineEditWithStatus::Ok, tr("The URL is ok."));
  }
  else if (!new_url.simplified().isEmpty()) {
    // New url is not well-formed but is not empty on the other hand.
    m_ui->m_txtUrl->setStatus(LineEditWithStatus::Warning,
                              tr(R"(The URL does not meet standard pattern. Does your URL start with "http://" or "https://" prefix.)"));
  }
  else {
    // New url is empty.
    m_ui->m_txtUrl->setStatus(LineEditWithStatus::Error, tr("The URL is empty."));
  }
}

void FormFeedDetails::onUsernameChanged(const QString& new_username) {
  bool is_username_ok = !m_ui->m_gbAuthentication->isChecked() || !new_username.simplified().isEmpty();

  m_ui->m_txtUsername->setStatus(is_username_ok ?
                                 LineEditWithStatus::Ok :
                                 LineEditWithStatus::Warning,
                                 is_username_ok ?
                                 tr("Username is ok or it is not needed.") :
                                 tr("Username is empty."));
}

void FormFeedDetails::onPasswordChanged(const QString& new_password) {
  bool is_password_ok = !m_ui->m_gbAuthentication->isChecked() || !new_password.simplified().isEmpty();

  m_ui->m_txtPassword->setStatus(is_password_ok ?
                                 LineEditWithStatus::Ok :
                                 LineEditWithStatus::Warning,
                                 is_password_ok ?
                                 tr("Password is ok or it is not needed.") :
                                 tr("Password is empty."));
}

void FormFeedDetails::onAuthenticationSwitched() {
  onUsernameChanged(m_ui->m_txtUsername->lineEdit()->text());
  onPasswordChanged(m_ui->m_txtPassword->lineEdit()->text());
}

void FormFeedDetails::onAutoUpdateTypeChanged(int new_index) {
  Feed::AutoUpdateType auto_update_type = static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(new_index).toInt());

  switch (auto_update_type) {
    case Feed::DontAutoUpdate:
    case Feed::DefaultAutoUpdate:
      m_ui->m_spinAutoUpdateInterval->setEnabled(false);
      break;

    case Feed::SpecificAutoUpdate:
    default:
      m_ui->m_spinAutoUpdateInterval->setEnabled(true);
  }
}

void FormFeedDetails::onLoadIconFromFile() {
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
    m_ui->m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void FormFeedDetails::onUseDefaultIcon() {
  m_ui->m_btnIcon->setIcon(QIcon());
}

void FormFeedDetails::apply() {}

void FormFeedDetails::guessFeed() {
  QPair<StandardFeed*, QNetworkReply::NetworkError> result = StandardFeed::guessFeed(m_ui->m_txtUrl->lineEdit()->text(),
                                                                                     m_ui->m_txtUsername->lineEdit()->text(),
                                                                                     m_ui->m_txtPassword->lineEdit()->text());

  if (result.first != nullptr) {
    // Icon or whole feed was guessed.
    m_ui->m_btnIcon->setIcon(result.first->icon());
    m_ui->m_txtTitle->lineEdit()->setText(result.first->title());
    m_ui->m_txtDescription->lineEdit()->setText(result.first->description());
    m_ui->m_cmbType->setCurrentIndex(m_ui->m_cmbType->findData(QVariant::fromValue((int) result.first->type())));
    int encoding_index = m_ui->m_cmbEncoding->findText(result.first->encoding(), Qt::MatchFixedString);

    if (encoding_index >= 0) {
      m_ui->m_cmbEncoding->setCurrentIndex(encoding_index);
    }
    else {
      m_ui->m_cmbEncoding->setCurrentIndex(m_ui->m_cmbEncoding->findText(DEFAULT_FEED_ENCODING,
                                                                         Qt::MatchFixedString));
    }

    if (result.second == QNetworkReply::NoError) {
      m_ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::Ok,
                                          tr("All metadata fetched successfully."),
                                          tr("Feed and icon metadata fetched."));
    }
    else {
      m_ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::Warning,
                                          tr("Result: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                          tr("Feed or icon metadata not fetched."));
    }

    // Remove temporary feed object.
    delete result.first;
  }
  else {
    // No feed guessed, even no icon available.
    m_ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::Error,
                                        tr("Error: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                        tr("No metadata fetched."));
  }
}

void FormFeedDetails::guessIconOnly() {
  QPair<StandardFeed*, QNetworkReply::NetworkError> result = StandardFeed::guessFeed(m_ui->m_txtUrl->lineEdit()->text(),
                                                                                     m_ui->m_txtUsername->lineEdit()->text(),
                                                                                     m_ui->m_txtPassword->lineEdit()->text());

  if (result.first != nullptr) {
    // Icon or whole feed was guessed.
    m_ui->m_btnIcon->setIcon(result.first->icon());

    if (result.second == QNetworkReply::NoError) {
      m_ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::Ok,
                                          tr("Icon fetched successfully."),
                                          tr("Icon metadata fetched."));
    }
    else {
      m_ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::Warning,
                                          tr("Result: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                          tr("Icon metadata not fetched."));
    }

    // Remove temporary feed object.
    delete result.first;
  }
  else {
    // No feed guessed, even no icon available.
    m_ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::Error,
                                        tr("Error: %1.").arg(NetworkFactory::networkErrorText(result.second)),
                                        tr("No icon fetched."));
  }
}

void FormFeedDetails::createConnections() {
  // General connections.
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormFeedDetails::apply);
  connect(m_ui->m_txtTitle->lineEdit(), &BaseLineEdit::textChanged, this, &FormFeedDetails::onTitleChanged);
  connect(m_ui->m_txtDescription->lineEdit(), &BaseLineEdit::textChanged, this, &FormFeedDetails::onDescriptionChanged);
  connect(m_ui->m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormFeedDetails::onUrlChanged);
  connect(m_ui->m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormFeedDetails::onUsernameChanged);
  connect(m_ui->m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &FormFeedDetails::onPasswordChanged);
  connect(m_ui->m_gbAuthentication, &QGroupBox::toggled, this, &FormFeedDetails::onAuthenticationSwitched);
  connect(m_ui->m_cmbAutoUpdateType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &FormFeedDetails::onAutoUpdateTypeChanged);
  connect(m_ui->m_btnFetchMetadata, &QPushButton::clicked, this, &FormFeedDetails::guessFeed);

  // Icon connections.
  connect(m_actionFetchIcon, &QAction::triggered, this, &FormFeedDetails::guessIconOnly);
  connect(m_actionLoadIconFromFile, &QAction::triggered, this, &FormFeedDetails::onLoadIconFromFile);
  connect(m_actionUseDefaultIcon, &QAction::triggered, this, &FormFeedDetails::onUseDefaultIcon);
}

void FormFeedDetails::setEditableFeed(Feed* editable_feed) {
  m_editableFeed = editable_feed;
  m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) editable_feed->parent())));
  m_ui->m_txtTitle->lineEdit()->setText(editable_feed->title());
  m_ui->m_txtDescription->lineEdit()->setText(editable_feed->description());
  m_ui->m_btnIcon->setIcon(editable_feed->icon());
  m_ui->m_txtUrl->lineEdit()->setText(editable_feed->url());
  m_ui->m_cmbAutoUpdateType->setCurrentIndex(m_ui->m_cmbAutoUpdateType->findData(QVariant::fromValue((int) editable_feed->autoUpdateType())));
  m_ui->m_spinAutoUpdateInterval->setValue(editable_feed->autoUpdateInitialInterval());
}

void FormFeedDetails::initialize() {
  m_ui.reset(new Ui::FormFeedDetails());
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("application-rss+xml")));

  // Set text boxes.
  m_ui->m_txtTitle->lineEdit()->setPlaceholderText(tr("Feed title"));
  m_ui->m_txtTitle->lineEdit()->setToolTip(tr("Set title for your feed."));
  m_ui->m_txtDescription->lineEdit()->setPlaceholderText(tr("Feed description"));
  m_ui->m_txtDescription->lineEdit()->setToolTip(tr("Set description for your feed."));
  m_ui->m_txtUrl->lineEdit()->setPlaceholderText(tr("Full feed url including scheme"));
  m_ui->m_txtUrl->lineEdit()->setToolTip(tr("Set url for your feed."));
  m_ui->m_txtUsername->lineEdit()->setPlaceholderText(tr("Username"));
  m_ui->m_txtUsername->lineEdit()->setToolTip(tr("Set username to access the feed."));
  m_ui->m_txtPassword->lineEdit()->setPlaceholderText(tr("Password"));
  m_ui->m_txtPassword->lineEdit()->setToolTip(tr("Set password to access the feed."));

  // Add standard feed types.
  m_ui->m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Atom10), QVariant::fromValue((int) StandardFeed::Atom10));
  m_ui->m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Rdf), QVariant::fromValue((int) StandardFeed::Rdf));
  m_ui->m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Rss0X), QVariant::fromValue((int) StandardFeed::Rss0X));
  m_ui->m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Rss2X), QVariant::fromValue((int) StandardFeed::Rss2X));

  // Load available encodings.
  const QList<QByteArray> encodings = QTextCodec::availableCodecs();
  QStringList encoded_encodings;

  foreach (const QByteArray& encoding, encodings) {
    encoded_encodings.append(encoding);
  }

  // Sort encodings and add them.
  qSort(encoded_encodings.begin(), encoded_encodings.end(), TextFactory::isCaseInsensitiveLessThan);
  m_ui->m_cmbEncoding->addItems(encoded_encodings);

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
  m_ui->m_btnIcon->setMenu(m_iconMenu);

  // Set feed metadata fetch label.
  m_ui->m_lblFetchMetadata->setStatus(WidgetWithStatus::Information,
                                      tr("No metadata fetched so far."),
                                      tr("No metadata fetched so far."));

  // Setup auto-update options.
  m_ui->m_spinAutoUpdateInterval->setValue(DEFAULT_AUTO_UPDATE_INTERVAL);
  m_ui->m_cmbAutoUpdateType->addItem(tr("Auto-update using global interval"), QVariant::fromValue((int) Feed::DefaultAutoUpdate));
  m_ui->m_cmbAutoUpdateType->addItem(tr("Auto-update every"), QVariant::fromValue((int) Feed::SpecificAutoUpdate));
  m_ui->m_cmbAutoUpdateType->addItem(tr("Do not auto-update at all"), QVariant::fromValue((int) Feed::DontAutoUpdate));

  // Set tab order.
  setTabOrder(m_ui->m_cmbParentCategory, m_ui->m_cmbType);
  setTabOrder(m_ui->m_cmbType, m_ui->m_cmbEncoding);
  setTabOrder(m_ui->m_cmbEncoding, m_ui->m_cmbAutoUpdateType);
  setTabOrder(m_ui->m_cmbAutoUpdateType, m_ui->m_spinAutoUpdateInterval);
  setTabOrder(m_ui->m_spinAutoUpdateInterval, m_ui->m_txtTitle->lineEdit());
  setTabOrder(m_ui->m_txtTitle->lineEdit(), m_ui->m_txtDescription->lineEdit());
  setTabOrder(m_ui->m_txtDescription->lineEdit(), m_ui->m_txtUrl->lineEdit());
  setTabOrder(m_ui->m_txtUrl->lineEdit(), m_ui->m_btnFetchMetadata);
  setTabOrder(m_ui->m_btnFetchMetadata, m_ui->m_btnIcon);
  setTabOrder(m_ui->m_btnIcon, m_ui->m_gbAuthentication);
  setTabOrder(m_ui->m_gbAuthentication, m_ui->m_txtUsername->lineEdit());
  setTabOrder(m_ui->m_txtUsername->lineEdit(), m_ui->m_txtPassword->lineEdit());
  m_ui->m_txtUrl->lineEdit()->setFocus(Qt::TabFocusReason);
}

void FormFeedDetails::loadCategories(const QList<Category*>& categories, RootItem* root_item) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(),
                                     root_item->title(),
                                     QVariant::fromValue((void*) root_item));

  foreach (Category* category, categories) {
    m_ui->m_cmbParentCategory->addItem(category->icon(),
                                       category->title(),
                                       QVariant::fromValue((void*) category));
  }
}
