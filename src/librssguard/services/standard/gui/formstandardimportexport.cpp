// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/gui/formstandardimportexport.h"

#include "core/feedsmodel.h"
#include "exceptions/ioexception.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/guiutilities.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "services/abstract/category.h"
#include "services/standard/standardfeedsimportexportmodel.h"
#include "services/standard/standardserviceroot.h"

#include <QFileDialog>
#include <QTextStream>

FormStandardImportExport::FormStandardImportExport(StandardServiceRoot* service_root, QWidget* parent)
  : QDialog(parent), m_ui(new Ui::FormStandardImportExport), m_serviceRoot(service_root) {
  m_ui->setupUi(this);
  m_model = new FeedsImportExportModel(m_ui->m_treeFeeds);
  connect(m_model, &FeedsImportExportModel::parsingStarted, this, &FormStandardImportExport::onParsingStarted);
  connect(m_model, &FeedsImportExportModel::parsingFinished, this, &FormStandardImportExport::onParsingFinished);
  connect(m_model, &FeedsImportExportModel::parsingProgress, this, &FormStandardImportExport::onParsingProgress);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("document-export")));

  m_ui->m_lblSelectFile->setStatus(WidgetWithStatus::StatusType::Error, tr("No file is selected."), tr("No file is selected."));
  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->disconnect();
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Warning, tr("No operation executed yet."), tr("No operation executed yet."));

  connect(m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok), &QPushButton::clicked, this, &FormStandardImportExport::performAction);
  connect(m_ui->m_btnSelectFile, &QPushButton::clicked, this, &FormStandardImportExport::selectFile);
  connect(m_ui->m_btnCheckAllItems, &QPushButton::clicked, m_model, &FeedsImportExportModel::checkAllItems);
  connect(m_ui->m_btnUncheckAllItems, &QPushButton::clicked, m_model, &FeedsImportExportModel::uncheckAllItems);
}

FormStandardImportExport::~FormStandardImportExport() = default;

void FormStandardImportExport::setMode(const FeedsImportExportModel::Mode& mode) {
  m_model->setMode(mode);
  m_ui->m_progressBar->setVisible(false);

  switch (mode) {
    case FeedsImportExportModel::Mode::Export: {
      m_model->setRootItem(m_serviceRoot);
      m_model->checkAllItems();
      m_ui->m_treeFeeds->setModel(m_model);
      m_ui->m_treeFeeds->expandAll();
      m_ui->m_cmbRootNode->setVisible(false);
      m_ui->m_lblRootNode->setVisible(false);
      m_ui->m_groupFile->setTitle(tr("Destination file"));
      m_ui->m_groupFeeds->setTitle(tr("Source feeds && categories"));
      m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setText(tr("&Export to file"));
      setWindowTitle(tr("Export feeds"));
      setWindowIcon(qApp->icons()->fromTheme(QSL("document-export")));
      selectExportFile(true);
      break;
    }

    case FeedsImportExportModel::Mode::Import: {
      m_ui->m_groupFile->setTitle(tr("Source file"));
      m_ui->m_groupFeeds->setTitle(tr("Target feeds && categories"));
      m_ui->m_groupFeeds->setDisabled(true);
      m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setText(tr("&Import from file"));

      // Load categories.
      loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);
      setWindowTitle(tr("Import feeds"));
      setWindowIcon(qApp->icons()->fromTheme(QSL("document-import")));
      break;
    }

    default:
      break;
  }

  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
}

void FormStandardImportExport::selectFile() {
  switch (m_model->mode()) {
    case FeedsImportExportModel::Mode::Import:
      selectImportFile();
      break;

    case FeedsImportExportModel::Mode::Export: {
      selectExportFile(false);
      break;
    }

    default:
      break;
  }
}

void FormStandardImportExport::onParsingStarted() {
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Progress, tr("Parsing data..."), tr("Parsing data..."));
  m_ui->m_btnSelectFile->setEnabled(false);
  m_ui->m_groupFeeds->setEnabled(false);
  m_ui->m_progressBar->setValue(0);
  m_ui->m_progressBar->setVisible(true);
  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
}

void FormStandardImportExport::onParsingFinished(int count_failed, int count_succeeded, bool parsing_error) {
  Q_UNUSED(count_failed)
  Q_UNUSED(count_succeeded)

  m_ui->m_progressBar->setVisible(false);
  m_ui->m_progressBar->setValue(0);
  m_model->checkAllItems();

  if (!parsing_error) {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Ok, tr("Feeds were loaded."), tr("Feeds were loaded."));
    m_ui->m_groupFeeds->setEnabled(true);
    m_ui->m_btnSelectFile->setEnabled(true);
    m_ui->m_treeFeeds->setModel(m_model);
    m_ui->m_treeFeeds->expandAll();
  }
  else {
    m_ui->m_groupFeeds->setEnabled(false);
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error, tr("Error, file is not well-formed. Select another file."),
                                 tr("Error occurred. File is not well-formed. Select another file."));
  }

  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(true);
}

void FormStandardImportExport::onParsingProgress(int completed, int total) {
  m_ui->m_progressBar->setMaximum(total);
  m_ui->m_progressBar->setValue(completed);
}

void FormStandardImportExport::selectExportFile(bool without_dialog) {
  const QString the_file = qApp->homeFolder() +
                           QDir::separator() +
                           QSL("rssguard_feeds_%1.opml").arg(QDate::currentDate().toString(Qt::DateFormat::ISODate));
  QString selected_file;
  QString selected_filter;
  const QString filter_opml20 = tr("OPML 2.0 files (*.opml)");
  const QString filter_txt_url_per_line = tr("TXT files [one URL per line] (*.txt)");

  if (!without_dialog) {
    QString filter;

    // Add more filters here.
    filter += filter_opml20;
    filter += ";;";
    filter += filter_txt_url_per_line;
    selected_file = QFileDialog::getSaveFileName(this, tr("Select file for feeds export"),
                                                 the_file,
                                                 filter,
                                                 &selected_filter);
  }
  else {
    selected_file = the_file;
    selected_filter = filter_opml20;
  }

  if (!selected_file.isEmpty()) {
    if (selected_filter == filter_opml20) {
      m_conversionType = ConversionType::OPML20;

      if (!selected_file.endsWith(QL1S(".opml"))) {
        selected_file += QL1S(".opml");
      }
    }
    else if (selected_filter == filter_txt_url_per_line) {
      m_conversionType = ConversionType::TxtUrlPerLine;

      if (!selected_file.endsWith(QL1S(".txt"))) {
        selected_file += QL1S(".txt");
      }
    }

    m_ui->m_lblSelectFile->setStatus(WidgetWithStatus::StatusType::Ok, QDir::toNativeSeparators(selected_file), tr("File is selected."));
  }

  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(m_ui->m_lblSelectFile->status() == WidgetWithStatus::StatusType::Ok);
}

void FormStandardImportExport::selectImportFile() {
  const QString filter_opml20 = tr("OPML 2.0 files (*.opml)");
  const QString filter_txt_url_per_line = tr("TXT files [one URL per line] (*.txt)");
  QString filter;
  QString selected_filter;

  // Add more filters here.
  filter += filter_opml20;
  filter += ";;";
  filter += filter_txt_url_per_line;
  const QString selected_file = QFileDialog::getOpenFileName(this, tr("Select file for feeds import"), qApp->homeFolder(),
                                                             filter, &selected_filter);

  if (!selected_file.isEmpty()) {
    if (selected_filter == filter_opml20) {
      m_conversionType = ConversionType::OPML20;
    }
    else if (selected_filter == filter_txt_url_per_line) {
      m_conversionType = ConversionType::TxtUrlPerLine;
    }

    m_ui->m_lblSelectFile->setStatus(WidgetWithStatus::StatusType::Ok, QDir::toNativeSeparators(selected_file), tr("File is selected."));
    QMessageBox::StandardButton answer = MessageBox::show(this,
                                                          QMessageBox::Icon::Warning,
                                                          tr("Get online metadata"),
                                                          tr("Metadata for your feeds can be fetched online. Note that the action "
                                                             "could take several minutes, depending on number of feeds."),
                                                          tr("Do you want to fetch feed metadata online?"),
                                                          QString(),
                                                          QMessageBox::StandardButton::Yes |
                                                          QMessageBox::StandardButton::No,
                                                          QMessageBox::StandardButton::Yes);

    parseImportFile(selected_file, answer == QMessageBox::StandardButton::Yes);
  }
}

void FormStandardImportExport::parseImportFile(const QString& file_name, bool fetch_metadata_online) {
  QFile input_file(file_name);
  QByteArray input_data;

  if (input_file.open(QIODevice::Text | QIODevice::Unbuffered | QIODevice::ReadOnly)) {
    input_data = input_file.readAll();
    input_file.close();
  }
  else {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error, tr("Cannot open source file."), tr("Cannot open source file."));
    return;
  }

  switch (m_conversionType) {
    case ConversionType::OPML20:
      m_model->importAsOPML20(input_data, fetch_metadata_online);
      break;

    case ConversionType::TxtUrlPerLine:
      m_model->importAsTxtURLPerLine(input_data, fetch_metadata_online);
      break;

    default:
      return;
  }
}

void FormStandardImportExport::performAction() {
  switch (m_model->mode()) {
    case FeedsImportExportModel::Mode::Import:
      importFeeds();
      break;

    case FeedsImportExportModel::Mode::Export:
      exportFeeds();
      break;

    default:
      break;
  }
}

void FormStandardImportExport::exportFeeds() {
  QByteArray result_data;
  bool result_export = false;

  switch (m_conversionType) {
    case ConversionType::OPML20:
      result_export = m_model->exportToOMPL20(result_data);
      break;

    case ConversionType::TxtUrlPerLine:
      result_export = m_model->exportToTxtURLPerLine(result_data);
      break;

    default:
      break;
  }

  if (result_export) {
    try {
      IOFactory::writeFile(m_ui->m_lblSelectFile->label()->text(), result_data);
      m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Ok, tr("Feeds were exported successfully."),
                                   tr("Feeds were exported successfully."));
    }
    catch (IOException& ex) {
      m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error, tr("Cannot write into destination file: '%1'."), ex.message());
    }
  }
  else {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error, tr("Critical error occurred."), tr("Critical error occurred."));
  }
}

void FormStandardImportExport::importFeeds() {
  QString output_message;
  RootItem* parent = static_cast<RootItem*>(m_ui->m_cmbRootNode->itemData(m_ui->m_cmbRootNode->currentIndex()).value<void*>());

  if (m_serviceRoot->mergeImportExportModel(m_model, parent, output_message)) {
    m_serviceRoot->requestItemExpand(parent->getSubTree(), true);
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Ok, output_message, output_message);
  }
  else {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error, output_message, output_message);
  }
}

void FormStandardImportExport::loadCategories(const QList<Category*>& categories, RootItem* root_item) {
  m_ui->m_cmbRootNode->addItem(root_item->icon(), root_item->title(), QVariant::fromValue((void*) root_item));

  for (Category* category : categories) {
    m_ui->m_cmbRootNode->addItem(category->icon(), category->title(), QVariant::fromValue((void*) category));
  }
}
