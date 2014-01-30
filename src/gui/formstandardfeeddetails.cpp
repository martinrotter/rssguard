#include "gui/formstandardfeeddetails.h"

#include "core/defs.h"
#include "core/textfactory.h"
#include "core/feedsmodel.h"
#include "core/feedsmodelrootitem.h"
#include "core/feedsmodelcategory.h"
#include "core/feedsmodelfeed.h"
#include "core/feedsmodelstandardfeed.h"
#include "gui/iconthemefactory.h"
#include "gui/baselineedit.h"

#if !defined(Q_OS_WIN)
#include "gui/messagebox.h"
#endif

#include <QPushButton>
#include <QTextCodec>
#include <QFileDialog>
#include <QMenu>


FormStandardFeedDetails::FormStandardFeedDetails(FeedsModel *model, QWidget *parent)
  : QDialog(parent),
    m_editableFeed(NULL),
    m_feedsModel(model) {
  initialize();
  createConnections();

  // Initialize that shit.
  onTitleChanged(QString());
  onDescriptionChanged(QString());
  onUrlChanged(QString());
}

FormStandardFeedDetails::~FormStandardFeedDetails() {
  delete m_ui;
}

int FormStandardFeedDetails::exec(FeedsModelStandardFeed *input_feed) {
  // Load categories.
  loadCategories(m_feedsModel->allCategories().values(),
                 m_feedsModel->rootItem(),
                 input_feed);

  if (input_feed == NULL) {
    // User is adding new category.
    setWindowTitle(tr("Add new standard feed"));

    // Make sure that "default" icon is used as the default option for new
    // feed.
    m_actionUseDefaultIcon->trigger();
  }
  else {
    // User is editing existing category.
    setWindowTitle(tr("Edit existing standard feed"));
    setEditableFeed(input_feed);
  }

  // Run the dialog.
  return QDialog::exec();
}

void FormStandardFeedDetails::onTitleChanged(const QString &new_title){
  if (new_title.simplified().size() >= MIN_CATEGORY_NAME_LENGTH) {
    m_ui->m_txtTitle->setStatus(LineEditWithStatus::Ok, tr("Feed name is ok."));
  }
  else {
    m_ui->m_txtTitle->setStatus(LineEditWithStatus::Error, tr("Feed name is too short."));
  }

  checkOkButtonEnabled();
}

void FormStandardFeedDetails::onDescriptionChanged(const QString &new_description) {
  if (new_description.simplified().isEmpty()) {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::Warning, tr("Description is empty."));
  }
  else {
    m_ui->m_txtDescription->setStatus(LineEditWithStatus::Ok, tr("The description os ok."));
  }
}

void FormStandardFeedDetails::onUrlChanged(const QString &new_url) {
  if (QRegExp(URL_REGEXP).exactMatch(new_url)) {
    // New url is well-formed.
    m_ui->m_txtUrl->setStatus(LineEditWithStatus::Ok, tr("The url is ok."));
  }
  else if (!new_url.simplified().isEmpty()) {
    // New url is not well-formed but is not empty on the other hand.
    m_ui->m_txtUrl->setStatus(LineEditWithStatus::Warning, tr("The url does not meet standard pattern. Does your url start with \"http://\" or \"https://\" prefix."));
  }
  else {
    // New url is empty.
    m_ui->m_txtUrl->setStatus(LineEditWithStatus::Error, tr("The url is empty."));
  }

  checkOkButtonEnabled();
}

void FormStandardFeedDetails::checkOkButtonEnabled() {
  LineEditWithStatus::StatusType title_status = m_ui->m_txtTitle->status();
  LineEditWithStatus::StatusType url_status = m_ui->m_txtUrl->status();

  m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(title_status == LineEditWithStatus::Ok &&
                                                              (url_status == LineEditWithStatus::Ok ||
                                                               url_status == LineEditWithStatus::Warning));
}

void FormStandardFeedDetails::onNoIconSelected() {
  m_ui->m_btnIcon->setIcon(QIcon());
}

void FormStandardFeedDetails::onLoadIconFromFile() {
  QFileDialog dialog(this, tr("Select icon file for the feed"),
                     QDir::homePath(), tr("Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)"));
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setWindowIcon(IconThemeFactory::instance()->fromTheme("image-x-generic"));
  dialog.setOptions(QFileDialog::DontUseNativeDialog | QFileDialog::ReadOnly);
  dialog.setViewMode(QFileDialog::Detail);
  dialog.setLabelText(QFileDialog::Accept, tr("Select icon"));
  dialog.setLabelText(QFileDialog::Reject, tr("Cancel"));
  dialog.setLabelText(QFileDialog::LookIn, tr("Look in:"));
  dialog.setLabelText(QFileDialog::FileName, tr("Icon name:"));
  dialog.setLabelText(QFileDialog::FileType, tr("Icon type:"));

  if (dialog.exec() == QDialog::Accepted) {
    m_ui->m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void FormStandardFeedDetails::onUseDefaultIcon() {
  m_ui->m_btnIcon->setIcon(IconThemeFactory::instance()->fromTheme("application-rss+xml"));
}

void FormStandardFeedDetails::apply() {
  FeedsModelRootItem *parent = static_cast<FeedsModelRootItem*>(m_ui->m_cmbParentCategory->itemData(m_ui->m_cmbParentCategory->currentIndex()).value<void*>());
  FeedsModelStandardFeed *new_feed = new FeedsModelStandardFeed();

  // TODO: Setup data for new_feed.

  if (m_editableFeed == NULL) {
    // TODO: Add the feed.
    // Add the category.
    if (m_feedsModel->addStandardFeed(new_feed, parent)) {
      accept();
    }
    else {
      // TODO: hlasit chybu
    }
  }
  else {
    // TODO: Edit the feed.
    if (m_feedsModel->editStandardFeed(m_editableFeed, new_feed)) {
      accept();
    }
    else {
      // TODO: hlasit chybu
    }
  }
}

void FormStandardFeedDetails::createConnections() {
  // General connections.
  connect(m_ui->m_buttonBox, SIGNAL(accepted()),
          this, SLOT(apply()));
  connect(m_ui->m_txtTitle->lineEdit(), SIGNAL(textChanged(QString)),
          this, SLOT(onTitleChanged(QString)));
  connect(m_ui->m_txtDescription->lineEdit(), SIGNAL(textChanged(QString)),
          this, SLOT(onDescriptionChanged(QString)));
  connect(m_ui->m_txtUrl->lineEdit(), SIGNAL(textChanged(QString)),
          this, SLOT(onUrlChanged(QString)));

  // Icon connections.
  connect(m_actionLoadIconFromFile, SIGNAL(triggered()), this, SLOT(onLoadIconFromFile()));
  connect(m_actionNoIcon, SIGNAL(triggered()), this, SLOT(onNoIconSelected()));
  connect(m_actionUseDefaultIcon, SIGNAL(triggered()), this, SLOT(onUseDefaultIcon()));
}

void FormStandardFeedDetails::setEditableFeed(FeedsModelStandardFeed *editable_feed) {
  m_editableFeed = editable_feed;

  m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) editable_feed->parent())));
  m_ui->m_txtTitle->lineEdit()->setText(editable_feed->title());
  m_ui->m_txtDescription->lineEdit()->setText(editable_feed->description());
  m_ui->m_btnIcon->setIcon(editable_feed->icon());
  m_ui->m_cmbType->setCurrentIndex(m_ui->m_cmbType->findData(QVariant::fromValue((void*) editable_feed->type())));
  m_ui->m_cmbEncoding->setCurrentIndex(m_ui->m_cmbEncoding->findData(editable_feed->encoding(), Qt::DisplayRole));
  m_ui->m_txtUrl->lineEdit()->setText(editable_feed->url());
}

void FormStandardFeedDetails::initialize() {
  m_ui = new Ui::FormStandardFeedDetails();
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconThemeFactory::instance()->fromTheme("document-new"));

  // Setup button box.
  m_ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  // Set text boxes.
  m_ui->m_txtTitle->lineEdit()->setPlaceholderText(tr("Feed title"));
  m_ui->m_txtTitle->lineEdit()->setToolTip(tr("Set title for your feed."));

  m_ui->m_txtDescription->lineEdit()->setPlaceholderText(tr("Feed description"));
  m_ui->m_txtDescription->lineEdit()->setToolTip(tr("Set description for your feed."));

  m_ui->m_txtUrl->lineEdit()->setPlaceholderText(tr("Full feed url including scheme"));
  m_ui->m_txtUrl->lineEdit()->setToolTip(tr("Set url for your feed."));

#if !defined(Q_OS_WIN)
  MessageBox::iconify(m_ui->m_buttonBox);
#endif

  // Add standard feed types.
  m_ui->m_cmbType->addItem(FeedsModelFeed::typeToString(FeedsModelFeed::StandardAtom10), QVariant::fromValue((void*) FeedsModelFeed::StandardAtom10));
  m_ui->m_cmbType->addItem(FeedsModelFeed::typeToString(FeedsModelFeed::StandardRdf), QVariant::fromValue((void*) FeedsModelFeed::StandardRdf));
  m_ui->m_cmbType->addItem(FeedsModelFeed::typeToString(FeedsModelFeed::StandardRss0X), QVariant::fromValue((void*) FeedsModelFeed::StandardRss0X));
  m_ui->m_cmbType->addItem(FeedsModelFeed::typeToString(FeedsModelFeed::StandardRss2X), QVariant::fromValue((void*) FeedsModelFeed::StandardRss2X));

  // Load available encodings.
  QList<QByteArray> encodings = QTextCodec::availableCodecs();
  QStringList encoded_encodings;

  foreach (const QByteArray &encoding, encodings) {
    encoded_encodings.append(encoding);
  }

  // Sort encodings and add them.
  qSort(encoded_encodings.begin(), encoded_encodings.end(), TextFactory::isCaseInsensitiveLessThan);
  m_ui->m_cmbEncoding->addItems(encoded_encodings);

  // Setup menu & actions for icon selection.
  m_iconMenu = new QMenu(tr("Icon selection"), this);
  m_actionLoadIconFromFile = new QAction(IconThemeFactory::instance()->fromTheme("image-x-generic"),
                                         tr("Load icon from file..."),
                                         this);
  m_actionNoIcon = new QAction(IconThemeFactory::instance()->fromTheme("edit-delete"),
                               tr("Do not use icon"),
                               this);
  m_actionUseDefaultIcon = new QAction(IconThemeFactory::instance()->fromTheme("application-rss+xml"),
                                       tr("Use default icon"),
                                       this);
  m_iconMenu->addAction(m_actionLoadIconFromFile);
  m_iconMenu->addAction(m_actionUseDefaultIcon);
  m_iconMenu->addAction(m_actionNoIcon);
  m_ui->m_btnIcon->setMenu(m_iconMenu);
}

void FormStandardFeedDetails::loadCategories(const QList<FeedsModelCategory*> categories,
                                             FeedsModelRootItem *root_item,
                                             FeedsModelStandardFeed *input_feed) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(),
                                     root_item->title(),
                                     QVariant::fromValue((void*) root_item));

  foreach (FeedsModelCategory *category, categories) {
    m_ui->m_cmbParentCategory->addItem(category->data(FDS_MODEL_TITLE_INDEX,
                                                      Qt::DecorationRole).value<QIcon>(),
                                       category->title(),
                                       QVariant::fromValue((void*) category));
  }
}
