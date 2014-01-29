#include "gui/formstandardfeeddetails.h"

#include "core/textfactory.h"
#include "core/feedsmodel.h"
#include "core/feedsmodelfeed.h"
#include "core/feedsmodelstandardfeed.h"
#include "gui/iconthemefactory.h"

#if !defined(Q_OS_WIN)
#include "gui/messagebox.h"
#endif

#include <QPushButton>
#include <QTextCodec>


FormStandardFeedDetails::FormStandardFeedDetails(FeedsModel *model, QWidget *parent)
  : QDialog(parent) {
  initialize();
}

FormStandardFeedDetails::~FormStandardFeedDetails() {
  delete m_ui;
}

int FormStandardFeedDetails::exec(FeedsModelStandardFeed *input_feed) {
  if (input_feed == NULL) {
    // User is adding new category.
    setWindowTitle(tr("Add new standard feed"));
  }
  else {
    // User is editing existing category.
    setWindowTitle(tr("Edit existing standard feed"));
    // TODO: set editable feed.
  }

  // TODO: Load categories.

  // Run the dialog.
  return QDialog::exec();
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
  m_ui->m_cmbType->addItem(FeedsModelFeed::typeToString(FeedsModelFeed::StandardAtom10), QVariant::fromValue(FeedsModelFeed::StandardAtom10));
  m_ui->m_cmbType->addItem(FeedsModelFeed::typeToString(FeedsModelFeed::StandardRdf), QVariant::fromValue(FeedsModelFeed::StandardRdf));
  m_ui->m_cmbType->addItem(FeedsModelFeed::typeToString(FeedsModelFeed::StandardRss0X), QVariant::fromValue(FeedsModelFeed::StandardRss0X));
  m_ui->m_cmbType->addItem(FeedsModelFeed::typeToString(FeedsModelFeed::StandardRss2X), QVariant::fromValue(FeedsModelFeed::StandardRss2X));

  // Load available encodings.
  QList<QByteArray> encodings = QTextCodec::availableCodecs();
  QStringList encoded_encodings;

  foreach (const QByteArray &encoding, encodings) {
    encoded_encodings.append(encoding);
  }

  qSort(encoded_encodings.begin(), encoded_encodings.end(), TextFactory::isCaseInsensitiveLessThan);
  m_ui->m_cmbEncoding->addItems(encoded_encodings);

}
