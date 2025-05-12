// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formstandardimportexport.h"

#include "src/standardfeedsimportexportmodel.h"
#include "src/standardserviceroot.h"

#include <librssguard/exceptions/ioexception.h>
#include <librssguard/gui/dialogs/filedialog.h>
#include <librssguard/gui/guiutilities.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/services/abstract/category.h>

#include <QTextStream>

FormStandardImportExport::FormStandardImportExport(StandardServiceRoot* service_root, QWidget* parent)
  : QDialog(parent), m_ui(new Ui::FormStandardImportExport), m_serviceRoot(service_root) {
  m_ui->setupUi(this);
  m_model = new FeedsImportExportModel(service_root, m_ui->m_treeFeeds);
  connect(m_model, &FeedsImportExportModel::parsingStarted, this, &FormStandardImportExport::onParsingStarted);
  connect(m_model, &FeedsImportExportModel::parsingFinished, this, &FormStandardImportExport::onParsingFinished);
  connect(m_model, &FeedsImportExportModel::parsingProgress, this, &FormStandardImportExport::onParsingProgress);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("document-export")));

  m_ui->m_txtPostProcessScript->textEdit()->setTabChangesFocus(true);
  m_ui->m_txtPostProcessScript->textEdit()->setPlaceholderText(tr("Full command to execute"));
  m_ui->m_txtPostProcessScript->textEdit()->setToolTip(tr("You can enter full command including interpreter here."));
  m_ui->m_txtPostProcessScript->setStatus(WidgetWithStatus::StatusType::Ok,
                                          tr("Here you can enter script executaion line, including interpreter."));

  m_ui->m_lblSelectFile->setStatus(WidgetWithStatus::StatusType::Error,
                                   tr("No file is selected."),
                                   tr("No file is selected."));
  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->disconnect();
  m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Warning,
                               tr("No operation executed yet."),
                               tr("No operation executed yet."));

  connect(m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok),
          &QPushButton::clicked,
          this,
          &FormStandardImportExport::performAction);
  connect(m_ui->m_btnSelectFile, &QPushButton::clicked, this, &FormStandardImportExport::selectFile);
  connect(m_ui->m_btnCheckAllItems, &QPushButton::clicked, m_model, &FeedsImportExportModel::checkAllItems);
  connect(m_ui->m_btnUncheckAllItems, &QPushButton::clicked, m_model, &FeedsImportExportModel::uncheckAllItems);
  connect(m_ui->m_txtPostProcessScript->textEdit(), &QPlainTextEdit::textChanged, this, [this]() {
    onPostProcessScriptChanged(m_ui->m_txtPostProcessScript->textEdit()->toPlainText());
  });

  onPostProcessScriptChanged({});
}

FormStandardImportExport::~FormStandardImportExport() = default;

void FormStandardImportExport::setMode(FeedsImportExportModel::Mode mode) {
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
      m_ui->m_groupFetchMetadata->setVisible(false);
      m_ui->m_groupFile->setTitle(tr("Destination file"));
      m_ui->m_groupFeeds->setTitle(tr("Source feeds && categories"));
      m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setText(tr("&Export to file"));
      setWindowTitle(tr("Export feeds"));
      setWindowIcon(qApp->icons()->fromTheme(QSL("document-export")));
      selectExportFile(true);
      break;
    }

    case FeedsImportExportModel::Mode::Import: {
      m_ui->m_cbExportIcons->setVisible(false);
      m_ui->m_groupFile->setTitle(tr("Source file"));
      m_ui->m_groupFeeds->setTitle(tr("Target feeds && categories"));
      m_ui->m_groupFeeds->setDisabled(true);
      m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setText(tr("&Import from file"));
      m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);

      // Load categories.
      loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);
      setWindowTitle(tr("Import feeds"));
      setWindowIcon(qApp->icons()->fromTheme(QSL("document-import")));
      break;
    }

    default:
      break;
  }
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
  m_ui->m_groupFetchMetadata->setEnabled(false);
  m_ui->m_progressBar->setValue(0);
  m_ui->m_progressBar->setVisible(true);
  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
}

void FormStandardImportExport::onParsingFinished(int count_failed, int count_succeeded) {
  m_ui->m_progressBar->setVisible(false);
  m_ui->m_progressBar->setValue(0);
  m_model->checkAllItems();

  if (count_failed > 0) {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Warning,
                                 tr("Some feeds were not loaded properly. Check log for more information."),
                                 tr("Some feeds were not loaded properly. Check log for more information."));
  }
  else {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Ok, tr("Feeds were loaded."), tr("Feeds were loaded."));
  }

  m_ui->m_groupFeeds->setEnabled(true);
  m_ui->m_groupFetchMetadata->setEnabled(true);
  m_ui->m_btnSelectFile->setEnabled(true);
  m_ui->m_treeFeeds->setModel(m_model);
  m_ui->m_treeFeeds->expandAll();

  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(true);
}

void FormStandardImportExport::onParsingProgress(int completed, int total) {
  m_ui->m_progressBar->setMaximum(total);
  m_ui->m_progressBar->setValue(completed);
}

void FormStandardImportExport::onPostProcessScriptChanged(const QString& new_pp) {
  if (QRegularExpression(QSL(SCRIPT_SOURCE_TYPE_REGEXP)).match(new_pp).hasMatch() || !new_pp.simplified().isEmpty()) {
    m_ui->m_txtPostProcessScript->setStatus(LineEditWithStatus::StatusType::Ok, tr("Command is ok."));
  }
  else {
    m_ui->m_txtPostProcessScript->setStatus(LineEditWithStatus::StatusType::Ok, tr("Command is empty."));
  }
}

void FormStandardImportExport::selectExportFile(bool without_dialog) {
  const QString the_file = qApp->homeFolder() + QDir::separator() +
                           QSL("rssguard_feeds_%1.opml").arg(QDate::currentDate().toString(Qt::DateFormat::ISODate));
  QString selected_file;
  QString selected_filter;
  const QString filter_opml20 = tr("OPML 2.0 files (*.opml *.xml)");
  const QString filter_txt_url_per_line = tr("TXT files [one URL per line] (*.txt)");

  if (!without_dialog) {
    QString filter;

    // Add more filters here.
    filter += filter_opml20;
    filter += QSL(";;");
    filter += filter_txt_url_per_line;
    selected_file = FileDialog::saveFileName(this,
                                             tr("Select file for feeds export"),
                                             the_file,
                                             filter,
                                             &selected_filter,
                                             GENERAL_REMEMBERED_PATH);
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

    m_ui->m_lblSelectFile->setStatus(WidgetWithStatus::StatusType::Ok,
                                     QDir::toNativeSeparators(selected_file),
                                     tr("File is selected."));
  }

  const auto is_ok = m_ui->m_lblSelectFile->status() == WidgetWithStatus::StatusType::Ok;

  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(is_ok);
}

void FormStandardImportExport::selectImportFile() {
  const QString filter_opml20 = tr("OPML 2.0 files (*.opml *.xml)").trimmed();
  const QString filter_txt_url_per_line = tr("TXT files [one URL per line] (*.txt)").trimmed();

  QString filter;
  QString selected_filter;

  // Add more filters here.
  filter += filter_opml20 + QSL(";;") + filter_txt_url_per_line;

  const QString selected_file = FileDialog::openFileName(this,
                                                         tr("Select file for feeds import"),
                                                         qApp->homeFolder(),
                                                         filter,
                                                         &selected_filter,
                                                         GENERAL_REMEMBERED_PATH);

  if (!selected_file.isEmpty()) {
    if (selected_filter == filter_opml20) {
      m_conversionType = ConversionType::OPML20;
    }
    else if (selected_filter == filter_txt_url_per_line) {
      m_conversionType = ConversionType::TxtUrlPerLine;
    }

    m_ui->m_cbDoNotFetchIcons->setEnabled(m_conversionType == ConversionType::OPML20);
    m_ui->m_cbDoNotFetchTitles->setEnabled(m_conversionType == ConversionType::OPML20);
    m_ui->m_lblSelectFile->setStatus(WidgetWithStatus::StatusType::Ok,
                                     QDir::toNativeSeparators(selected_file),
                                     tr("File is selected."));

    try {
      parseImportFile(selected_file,
                      m_ui->m_groupFetchMetadata->isChecked(),
                      m_ui->m_cbDoNotFetchTitles->isChecked(),
                      m_ui->m_cbDoNotFetchIcons->isChecked(),
                      m_ui->m_txtPostProcessScript->textEdit()->toPlainText());
    }
    catch (const ApplicationException& ex) {
      m_ui->m_btnSelectFile->setEnabled(true);
      m_ui->m_progressBar->setVisible(false);
      m_ui->m_progressBar->setValue(0);
      m_ui->m_groupFeeds->setEnabled(false);
      m_ui->m_groupFetchMetadata->setEnabled(true);

      m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error, ex.message(), ex.message());
    }
  }
}

void FormStandardImportExport::parseImportFile(const QString& file_name,
                                               bool fetch_metadata_online,
                                               bool do_not_fetch_titles,
                                               bool do_not_fetch_icons,
                                               const QString& post_process_script) {
  QByteArray input_data;
  QFile input_file(file_name);

  if (input_file.open(QIODevice::OpenModeFlag::Text | QIODevice::OpenModeFlag::Unbuffered |
                      QIODevice::OpenModeFlag::ReadOnly)) {
    input_data = input_file.readAll();
    input_file.close();
  }
  else {
    throw ApplicationException(tr("cannot open file"));
  }

  switch (m_conversionType) {
    case ConversionType::OPML20:
      m_model->importAsOPML20(input_data,
                              fetch_metadata_online,
                              do_not_fetch_titles,
                              do_not_fetch_icons,
                              post_process_script);
      break;

    case ConversionType::TxtUrlPerLine:
      m_model->importAsTxtURLPerLine(input_data,
                                     fetch_metadata_online,
                                     m_ui->m_txtPostProcessScript->textEdit()->toPlainText());
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
      result_export = m_model->exportToOMPL20(result_data, m_ui->m_cbExportIcons->isChecked());
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
      m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                   tr("Feeds were exported successfully."),
                                   tr("Feeds were exported successfully."));
    }
    catch (IOException& ex) {
      m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error,
                                   tr("Cannot write into destination file: '%1'."),
                                   ex.message());
    }
  }
  else {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error,
                                 tr("Critical error occurred."),
                                 tr("Critical error occurred."));
  }
}

void FormStandardImportExport::importFeeds() {
  QString output_message;
  RootItem* parent = m_ui->m_cmbRootNode->currentData().value<RootItem*>();

  if (m_serviceRoot->mergeImportExportModel(m_model, parent, output_message)) {
    m_serviceRoot->requestItemExpand(parent->getSubTree<RootItem>(), true);
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Ok, output_message, output_message);
  }
  else {
    m_ui->m_lblResult->setStatus(WidgetWithStatus::StatusType::Error, output_message, output_message);
  }
}

void FormStandardImportExport::loadCategories(const QList<Category*>& categories, RootItem* root_item) {
  m_ui->m_cmbRootNode->addItem(root_item->icon(), root_item->title(), QVariant::fromValue(root_item));

  for (Category* category : categories) {
    m_ui->m_cmbRootNode->addItem(category->icon(), category->title(), QVariant::fromValue(category));
  }
}
