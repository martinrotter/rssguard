// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/standardfeeddetails.h"

#include "src/definitions.h"

#include <librssguard/3rd-party/boolinq/boolinq.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/exceptions/networkexception.h>
#include <librssguard/exceptions/scriptexception.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/services/abstract/category.h>

#include <QFileDialog>
#include <QImageReader>
#include <QMenu>
#include <QMimeData>
#include <QTextCodec>
#include <QtGlobal>

StandardFeedDetails::StandardFeedDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  m_ui.m_txtPostProcessScript->textEdit()->setTabChangesFocus(true);
  m_ui.m_txtSource->textEdit()->setTabChangesFocus(true);

  m_ui.m_txtTitle->lineEdit()->setPlaceholderText(tr("Feed title"));
  m_ui.m_txtTitle->lineEdit()->setToolTip(tr("Set title for your feed."));
  m_ui.m_txtDescription->lineEdit()->setPlaceholderText(tr("Feed description"));
  m_ui.m_txtDescription->lineEdit()->setToolTip(tr("Set description for your feed."));
  m_ui.m_txtSource->textEdit()->setPlaceholderText(tr("Full feed source identifier"));
  m_ui.m_txtSource->textEdit()->setToolTip(tr("Full feed source identifier which can be URL."));
  m_ui.m_txtPostProcessScript->textEdit()->setPlaceholderText(tr("Full command to execute"));
  m_ui.m_txtPostProcessScript->textEdit()->setToolTip(tr("You can enter full command including interpreter here."));

  // Add source types.
  m_ui.m_cmbSourceType->addItem(StandardFeed::sourceTypeToString(StandardFeed::SourceType::Url),
                                QVariant::fromValue(StandardFeed::SourceType::Url));
  m_ui.m_cmbSourceType->addItem(StandardFeed::sourceTypeToString(StandardFeed::SourceType::Script),
                                QVariant::fromValue(StandardFeed::SourceType::Script));
  m_ui.m_cmbSourceType->addItem(StandardFeed::sourceTypeToString(StandardFeed::SourceType::LocalFile),
                                QVariant::fromValue(StandardFeed::SourceType::LocalFile));
  m_ui.m_cmbSourceType->addItem(StandardFeed::sourceTypeToString(StandardFeed::SourceType::EmbeddedBrowser),
                                QVariant::fromValue(StandardFeed::SourceType::EmbeddedBrowser));

  // Add standard feed types.
  m_ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Atom10),
                          QVariant::fromValue(int(StandardFeed::Type::Atom10)));
  m_ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Rdf),
                          QVariant::fromValue(int(StandardFeed::Type::Rdf)));
  m_ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Rss0X),
                          QVariant::fromValue(int(StandardFeed::Type::Rss0X)));
  m_ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Rss2X),
                          QVariant::fromValue(int(StandardFeed::Type::Rss2X)));
  m_ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::iCalendar),
                          QVariant::fromValue(int(StandardFeed::Type::iCalendar)));
  m_ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Json),
                          QVariant::fromValue(int(StandardFeed::Type::Json)));
  m_ui.m_cmbType->addItem(StandardFeed::typeToString(StandardFeed::Type::Sitemap),
                          QVariant::fromValue(int(StandardFeed::Type::Sitemap)));

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

  m_ui.m_cmbEncoding->addItems(encoded_encodings);

  // Setup menu & actions for icon selection.
  m_iconMenu = new QMenu(tr("Icon selection"), this);
  m_actionLoadIconFromFile =
    new QAction(qApp->icons()->fromTheme(QSL("image-x-generic")), tr("Load icon from file..."), this);
  m_actionUseDefaultIcon =
    new QAction(qApp->icons()->fromTheme(QSL("application-rss+xml")), tr("Use default icon from icon theme"), this);
  m_actionFetchIcon =
    new QAction(qApp->icons()->fromTheme(QSL("emblem-downloads"), QSL("download")), tr("Fetch icon from feed"), this);
  m_iconMenu->addAction(m_actionFetchIcon);
  m_iconMenu->addAction(m_actionLoadIconFromFile);
  m_iconMenu->addAction(m_actionUseDefaultIcon);
  m_ui.m_btnIcon->setMenu(m_iconMenu);
  m_ui.m_txtSource->textEdit()->setFocus(Qt::FocusReason::TabFocusReason);

  // Set feed metadata fetch label.
  m_ui.m_lblFetchMetadata->label()->setWordWrap(true);
  m_ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Information,
                                     tr("No metadata fetched so far."),
                                     tr("No metadata fetched so far."));

  connect(m_ui.m_txtTitle->lineEdit(), &BaseLineEdit::textChanged, this, &StandardFeedDetails::onTitleChanged);
  connect(m_ui.m_txtDescription->lineEdit(),
          &BaseLineEdit::textChanged,
          this,
          &StandardFeedDetails::onDescriptionChanged);
  connect(m_ui.m_cmbSourceType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
    onUrlChanged(m_ui.m_txtSource->textEdit()->toPlainText());
  });
  connect(m_ui.m_txtSource->textEdit(), &QPlainTextEdit::textChanged, this, [this]() {
    onUrlChanged(m_ui.m_txtSource->textEdit()->toPlainText());
  });
  connect(m_ui.m_txtPostProcessScript->textEdit(), &QPlainTextEdit::textChanged, this, [this]() {
    onPostProcessScriptChanged(m_ui.m_txtPostProcessScript->textEdit()->toPlainText());
  });
  connect(m_actionLoadIconFromFile, &QAction::triggered, this, &StandardFeedDetails::onLoadIconFromFile);
  connect(m_actionUseDefaultIcon, &QAction::triggered, this, &StandardFeedDetails::onUseDefaultIcon);

  setTabOrder(m_ui.m_cmbParentCategory, m_ui.m_cmbType);
  setTabOrder(m_ui.m_cmbType, m_ui.m_cmbEncoding);
  setTabOrder(m_ui.m_cmbEncoding, m_ui.m_txtTitle->lineEdit());
  setTabOrder(m_ui.m_txtTitle->lineEdit(), m_ui.m_txtDescription->lineEdit());
  setTabOrder(m_ui.m_txtDescription->lineEdit(), m_ui.m_cmbSourceType);
  setTabOrder(m_ui.m_cmbSourceType, m_ui.m_txtSource->textEdit());
  setTabOrder(m_ui.m_txtSource->textEdit(), m_ui.m_txtPostProcessScript->textEdit());
  setTabOrder(m_ui.m_txtPostProcessScript->textEdit(), m_ui.m_btnFetchMetadata);
  setTabOrder(m_ui.m_btnFetchMetadata, m_ui.m_btnIcon);

  m_ui.m_lblScriptInfo->setHelpText(tr("What is post-processing script?"),
                                    tr("You can use URL as a source of your feed or you can produce your feed with "
                                       "custom script.\n\n"
                                       "Also, you can post-process generated feed data with yet "
                                       "another script if you wish. These are advanced features and make sure to "
                                       "read the documentation before your use them."),
                                    true);

  onTitleChanged({});
  onDescriptionChanged({});
  onUrlChanged({});
  onPostProcessScriptChanged({});
}

void StandardFeedDetails::guessIconOnly(StandardFeed::SourceType source_type,
                                        const QString& source,
                                        const QString& post_process_script,
                                        NetworkFactory::NetworkAuthentication protection,
                                        const QString& username,
                                        const QString& password,
                                        const QList<QPair<QByteArray, QByteArray>>& headers,
                                        const QNetworkProxy& custom_proxy) {
  try {
    StandardFeed* metadata = StandardFeed::guessFeed(source_type,
                                                     source,
                                                     post_process_script,
                                                     protection,
                                                     true,
                                                     username,
                                                     password,
                                                     {},
                                                     custom_proxy);

    // Icon or whole feed was guessed.
    m_ui.m_btnIcon->setIcon(metadata->icon());
    m_ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Ok,
                                       tr("Icon fetched successfully."),
                                       tr("Icon metadata fetched."));

    // Remove temporary feed object.
    metadata->deleteLater();
  }
  catch (const ScriptException& ex) {
    m_ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                       tr("Script failed: %1").arg(ex.message()),
                                       tr("No icon fetched."));
  }
  catch (const NetworkException& ex) {
    m_ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                       tr("Network error: %1").arg(ex.message()),
                                       tr("No icon fetched."));
  }
  catch (const ApplicationException& ex) {
    m_ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                       tr("Error: %1").arg(ex.message()),
                                       tr("No icon fetched."));
  }
}

void StandardFeedDetails::guessFeed(StandardFeed::SourceType source_type,
                                    const QString& source,
                                    const QString& post_process_script,
                                    NetworkFactory::NetworkAuthentication protection,
                                    const QString& username,
                                    const QString& password,
                                    const QList<QPair<QByteArray, QByteArray>>& headers,
                                    const QNetworkProxy& custom_proxy) {
  try {
    StandardFeed* metadata = StandardFeed::guessFeed(source_type,
                                                     source,
                                                     post_process_script,
                                                     protection,
                                                     true,
                                                     username,
                                                     password,
                                                     headers,
                                                     custom_proxy);

    // Icon or whole feed was guessed.
    m_ui.m_btnIcon->setIcon(metadata->icon());
    m_ui.m_txtTitle->lineEdit()->setText(metadata->sanitizedTitle());
    m_ui.m_txtDescription->lineEdit()->setText(metadata->description());
    m_ui.m_cmbType->setCurrentIndex(m_ui.m_cmbType->findData(QVariant::fromValue((int)metadata->type())));
    int encoding_index = m_ui.m_cmbEncoding->findText(metadata->encoding(), Qt::MatchFlag::MatchFixedString);

    if (encoding_index >= 0) {
      m_ui.m_cmbEncoding->setCurrentIndex(encoding_index);
    }
    else {
      m_ui.m_cmbEncoding->setCurrentIndex(m_ui.m_cmbEncoding->findText(QSL(DEFAULT_FEED_ENCODING),
                                                                       Qt::MatchFlag::MatchFixedString));
    }

    m_ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Ok,
                                       tr("All metadata fetched successfully."),
                                       tr("Feed and icon metadata fetched."));

    // Remove temporary feed object.
    metadata->deleteLater();
  }
  catch (const ScriptException& ex) {
    m_ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                       tr("Script failed: %1").arg(ex.message()),
                                       tr("No metadata fetched."));
  }
  catch (const NetworkException& ex) {
    m_ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                       tr("Network error: %1").arg(ex.message()),
                                       tr("No metadata fetched."));
  }
  catch (const ApplicationException& ex) {
    m_ui.m_lblFetchMetadata->setStatus(WidgetWithStatus::StatusType::Error,
                                       tr("Error: %1").arg(ex.message()),
                                       tr("No metadata fetched."));
  }
}

void StandardFeedDetails::onTitleChanged(const QString& new_title) {
  if (!new_title.simplified().isEmpty()) {
    m_ui.m_txtTitle->setStatus(LineEditWithStatus::StatusType::Ok, tr("Feed name is ok."));
  }
  else {
    m_ui.m_txtTitle->setStatus(LineEditWithStatus::StatusType::Error, tr("Feed name is too short."));
  }
}

void StandardFeedDetails::onDescriptionChanged(const QString& new_description) {
  if (new_description.simplified().isEmpty()) {
    m_ui.m_txtDescription->setStatus(LineEditWithStatus::StatusType::Warning, tr("Description is empty."));
  }
  else {
    m_ui.m_txtDescription->setStatus(LineEditWithStatus::StatusType::Ok, tr("The description is ok."));
  }
}

void StandardFeedDetails::onUrlChanged(const QString& new_url) {
  switch (sourceType()) {
    case StandardFeed::SourceType::EmbeddedBrowser:
    case StandardFeed::SourceType::Url: {
      if (QUrl(new_url).isValid()) {
        m_ui.m_txtSource->setStatus(LineEditWithStatus::StatusType::Ok, tr("The URL is ok."));
      }
      else if (!new_url.simplified().isEmpty()) {
        m_ui.m_txtSource->setStatus(LineEditWithStatus::StatusType::Warning,
                                    tr("The URL does not meet standard pattern. "
                                       "Does your URL start with \"http://\" or \"https://\" prefix."));
      }
      else {
        m_ui.m_txtSource->setStatus(LineEditWithStatus::StatusType::Error, tr("The URL is empty."));
      }

      break;
    }

    case StandardFeed::SourceType::Script: {
      try {
        TextFactory::tokenizeProcessArguments(new_url);
        m_ui.m_txtSource->setStatus(LineEditWithStatus::StatusType::Ok, tr("Source is ok."));
      }
      catch (const ApplicationException& ex) {
        m_ui.m_txtSource->setStatus(LineEditWithStatus::StatusType::Error, tr("Error: %1").arg(ex.message()));
      }

      break;
    }
    case StandardFeed::SourceType::LocalFile: {
      if (QFile::exists(new_url)) {
        m_ui.m_txtSource->setStatus(LineEditWithStatus::StatusType::Ok, tr("File exists."));
      }
      else {
        m_ui.m_txtSource->setStatus(LineEditWithStatus::StatusType::Error, tr("File does not exist."));
      }

      break;
    }

    default:
      m_ui.m_txtSource->setStatus(LineEditWithStatus::StatusType::Ok, tr("The source is ok."));
  }
}

void StandardFeedDetails::onPostProcessScriptChanged(const QString& new_pp) {
  try {
    TextFactory::tokenizeProcessArguments(new_pp);
    m_ui.m_txtPostProcessScript->setStatus(LineEditWithStatus::StatusType::Ok, tr("Command is ok."));
  }
  catch (const ApplicationException& ex) {
    m_ui.m_txtPostProcessScript->setStatus(LineEditWithStatus::StatusType::Error, tr("Error: %1").arg(ex.message()));
  }
}

void StandardFeedDetails::onLoadIconFromFile() {
  auto supported_formats = QImageReader::supportedImageFormats();
  auto prefixed_formats = boolinq::from(supported_formats)
                            .select([](const QByteArray& frmt) {
                              return QSL("*.%1").arg(QString::fromLocal8Bit(frmt));
                            })
                            .toStdList();

  QStringList list_formats = FROM_STD_LIST(QStringList, prefixed_formats);

  QFileDialog dialog(this,
                     tr("Select icon file for the feed"),
                     qApp->homeFolder(),
                     tr("Images (%1)").arg(list_formats.join(QL1C(' '))));

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
    m_ui.m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void StandardFeedDetails::onUseDefaultIcon() {
  m_ui.m_btnIcon->setIcon(QIcon());
}

StandardFeed::SourceType StandardFeedDetails::sourceType() const {
  return m_ui.m_cmbSourceType->currentData().value<StandardFeed::SourceType>();
}

void StandardFeedDetails::prepareForNewFeed(RootItem* parent_to_select, const QString& url) {
  // Make sure that "default" icon is used as the default option for new
  // feed.
  m_actionUseDefaultIcon->trigger();

  int default_encoding_index = m_ui.m_cmbEncoding->findText(QSL(DEFAULT_FEED_ENCODING));

  if (default_encoding_index >= 0) {
    m_ui.m_cmbEncoding->setCurrentIndex(default_encoding_index);
  }

  if (parent_to_select != nullptr) {
    if (parent_to_select->kind() == RootItem::Kind::Category) {
      m_ui.m_cmbParentCategory
        ->setCurrentIndex(m_ui.m_cmbParentCategory->findData(QVariant::fromValue(parent_to_select)));
    }
    else if (parent_to_select->kind() == RootItem::Kind::Feed) {
      int target_item = m_ui.m_cmbParentCategory->findData(QVariant::fromValue(parent_to_select->parent()));

      if (target_item >= 0) {
        m_ui.m_cmbParentCategory->setCurrentIndex(target_item);
      }
    }
    else {
      m_ui.m_cmbParentCategory->setCurrentIndex(0);
    }
  }

  if (!url.isEmpty()) {
    m_ui.m_txtSource->textEdit()->setPlainText(url);
  }

  m_ui.m_txtSource->setFocus();
  m_ui.m_txtSource->textEdit()->selectAll();
}

void StandardFeedDetails::setExistingFeed(StandardFeed* feed) {
  m_ui.m_cmbSourceType->setCurrentIndex(m_ui.m_cmbSourceType->findData(QVariant::fromValue(feed->sourceType())));
  m_ui.m_cmbParentCategory->setCurrentIndex(m_ui.m_cmbParentCategory->findData(QVariant::fromValue(feed->parent())));
  m_ui.m_txtTitle->lineEdit()->setText(feed->title());
  m_ui.m_txtDescription->lineEdit()->setText(feed->description());
  m_ui.m_btnIcon->setIcon(feed->icon());
  m_ui.m_txtSource->textEdit()->setPlainText(feed->source());
  m_ui.m_txtPostProcessScript->textEdit()->setPlainText(feed->postProcessScript());
  m_ui.m_cmbType->setCurrentIndex(m_ui.m_cmbType->findData(QVariant::fromValue(int(feed->type()))));
  m_ui.m_cmbEncoding->setCurrentIndex(m_ui.m_cmbEncoding->findData(feed->encoding(),
                                                                   Qt::ItemDataRole::DisplayRole,
                                                                   Qt::MatchFlag::MatchFixedString));
}

void StandardFeedDetails::loadCategories(const QList<Category*>& categories, RootItem* root_item) {
  m_ui.m_cmbParentCategory->addItem(root_item->fullIcon(), root_item->title(), QVariant::fromValue(root_item));

  for (Category* category : categories) {
    m_ui.m_cmbParentCategory->addItem(category->fullIcon(), category->title(), QVariant::fromValue(category));
  }
}
