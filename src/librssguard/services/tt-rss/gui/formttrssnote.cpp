// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/gui/formttrssnote.h"

#include "gui/guiutilities.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssnotetopublish.h"
#include "services/tt-rss/ttrssserviceroot.h"

FormTtRssNote::FormTtRssNote(TtRssServiceRoot* root) : QDialog(qApp->mainFormWidget()), m_root(root), m_titleOk(false),
  m_urlOk(false) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->fromTheme(QSL("emblem-shared")),
                                      tr("Share note to \"Published\" feed"));

  setTabOrder(m_ui.m_txtTitle->lineEdit(), m_ui.m_txtUrl->lineEdit());
  setTabOrder(m_ui.m_txtUrl->lineEdit(), m_ui.m_txtContent);
  setTabOrder(m_ui.m_txtContent, m_ui.m_btnBox);

  connect(m_ui.m_txtTitle->lineEdit(), &BaseLineEdit::textChanged, this, &FormTtRssNote::onTitleChanged);
  connect(m_ui.m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormTtRssNote::onUrlChanged);
  connect(m_ui.m_btnBox, &QDialogButtonBox::accepted, this, &FormTtRssNote::sendNote);

  emit m_ui.m_txtTitle->lineEdit()->textChanged({});
  emit m_ui.m_txtUrl->lineEdit()->textChanged({});
}

void FormTtRssNote::sendNote() {
  TtRssNoteToPublish note;

  note.m_content = m_ui.m_txtContent->toPlainText();
  note.m_url = m_ui.m_txtUrl->lineEdit()->text();
  note.m_title = m_ui.m_txtTitle->lineEdit()->text();

  auto res = m_root->network()->shareToPublished(note, m_root->networkProxy());

  if (res.status() == TTRSS_API_STATUS_OK) {
    accept();
  }
  else {
    MsgBox::show({}, QMessageBox::Icon::Critical,
                     tr("Cannot share note"),
                     tr("There was an error, when trying to send your custom note."),
                     {},
                     res.error());
  }
}

void FormTtRssNote::onTitleChanged(const QString& text) {
  m_titleOk = !text.simplified().isEmpty();

  m_ui.m_txtTitle->setStatus(m_titleOk
                             ? WidgetWithStatus::StatusType::Ok
                             : WidgetWithStatus::StatusType::Error,
                             tr("Enter non-empty title."));

  updateOkButton();
}

void FormTtRssNote::onUrlChanged(const QString& text) {
  m_urlOk = text.startsWith(URI_SCHEME_HTTPS) || text.startsWith(URI_SCHEME_HTTP);

  m_ui.m_txtUrl->setStatus(m_urlOk
                           ? WidgetWithStatus::StatusType::Ok
                           : WidgetWithStatus::StatusType::Error,
                           tr("Enter valid URL."));

  updateOkButton();
}

void FormTtRssNote::updateOkButton() {
  m_ui.m_btnBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(m_urlOk && m_titleOk);
}
