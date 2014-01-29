#include "gui/formstandardfeeddetails.h"

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


FormStandardFeedDetails::FormStandardFeedDetails(FeedsModel *model, QWidget *parent)
  : QDialog(parent),
    m_editableFeed(NULL),
    m_feedsModel(model) {
  initialize();
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
  }
  else {
    // User is editing existing category.
    setWindowTitle(tr("Edit existing standard feed"));
    setEditableFeed(input_feed);
  }

  // Run the dialog.
  return QDialog::exec();
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

  qSort(encoded_encodings.begin(), encoded_encodings.end(), TextFactory::isCaseInsensitiveLessThan);
  m_ui->m_cmbEncoding->addItems(encoded_encodings);
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
