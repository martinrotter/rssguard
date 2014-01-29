#include "gui/formstandardfeeddetails.h"

#include "core/feedsmodel.h"
#include "gui/iconthemefactory.h"

#if !defined(Q_OS_WIN)
#include "gui/messagebox.h"
#endif

#include <QPushButton>

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
  }

  return 0;
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
}
