// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gui/formaddeditemail.h"

#include "3rd-party/mimesis/mimesis.hpp"
#include "database/databasequeries.h"
#include "exceptions/applicationexception.h"
#include "gui/guiutilities.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/gmail/gmailnetworkfactory.h"
#include "services/gmail/gmailserviceroot.h"
#include "services/gmail/gui/emailrecipientcontrol.h"

#include <QCloseEvent>
#include <QTextDocumentFragment>

FormAddEditEmail::FormAddEditEmail(GmailServiceRoot* root, QWidget* parent)
  : QDialog(parent), m_root(root), m_originalMessage(nullptr), m_possibleRecipients({}) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("mail-message-new")));

  m_ui.m_layoutAdder->setContentsMargins({});

  m_ui.m_btnAdder->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_ui.m_btnAdder->setToolTip(tr("Add new recipient."));
  m_ui.m_btnAdder->setFocusPolicy(Qt::FocusPolicy::NoFocus);

  connect(m_ui.m_btnAdder, &PlainToolButton::clicked, this, [=]() {
    addRecipientRow();
  });

  connect(m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Ok),
          &QPushButton::clicked,
          this,
          &FormAddEditEmail::onOkClicked);

  QSqlDatabase db = qApp->database()->driver()->connection(QSL("FormAddEditEmail"));

  m_possibleRecipients = DatabaseQueries::getAllGmailRecipients(db, m_root->accountId());
  auto ctrls = recipientControls();

  for (auto* rec : std::as_const(ctrls)) {
    rec->setPossibleRecipients(m_possibleRecipients);
  }
}

void FormAddEditEmail::execForAdd() {
  addRecipientRow()->setFocus();
  exec();
}

void FormAddEditEmail::execForReply(Message* original_message) {
  m_originalMessage = original_message;

  m_ui.m_txtSubject->setText(QSL("Re: %1").arg(m_originalMessage->m_title));
  m_ui.m_txtSubject->setEnabled(false);
  m_ui.m_txtMessage->setFocus();

  auto from_header =
    m_root->network()->getMessageMetadata(original_message->m_customId, {QSL("FROM")}, m_root->networkProxy());

  // TODO: konverze html > plain
  // QTextDocumentFragment::fromHtml(m_originalMessage->m_contents).toPlainText()
  m_ui.m_txtMessage->setText(m_originalMessage->m_contents);
  m_ui.m_txtMessage->editor()->moveCursor(QTextCursor::MoveOperation::Start);
  m_ui.m_txtMessage->editor()->insertHtml(QSL("<p>"
                                              "---------- Original message ----------"
                                              "</p><br/>"));
  m_ui.m_txtMessage->editor()->moveCursor(QTextCursor::MoveOperation::Start);

  addRecipientRow(from_header[QSL("From")]);
  exec();
}

void FormAddEditEmail::execForForward(Message* original_message) {
  m_originalMessage = original_message;

  m_ui.m_txtSubject->setText(QSL("Fwd: %1").arg(m_originalMessage->m_title));
  m_ui.m_txtSubject->setEnabled(false);
  m_ui.m_txtMessage->setFocus();

  const QString to_header =
    m_root->network()->getMessageMetadata(original_message->m_customId, {QSL("TO")}, m_root->networkProxy())["To"];

  const QString forward_header =
    QSL("<pre>"
        "---------- Forwarded message ---------<br/>"
        "From: %1<br/>"
        "Date: %2<br/>"
        "Subject: %3<br/>"
        "To: %4"
        "</pre><br/>")
      .arg(m_originalMessage->m_author, m_originalMessage->m_created.toString(), m_originalMessage->m_title, to_header);

  m_ui.m_txtMessage->setText(forward_header + m_originalMessage->m_contents);
  m_ui.m_txtMessage->editor()->moveCursor(QTextCursor::MoveOperation::Start);

  addRecipientRow()->setFocus();
  exec();
}

void FormAddEditEmail::removeRecipientRow() {
  auto* sndr = static_cast<EmailRecipientControl*>(sender());

  m_ui.m_layout->takeRow(sndr);
  m_recipientControls.removeOne(sndr);

  sndr->deleteLater();
}

void FormAddEditEmail::onOkClicked() {
  Mimesis::Message msg;
  auto username = m_root->network()->username();

  if (!username.endsWith(QSL("@gmail.com"))) {
    username += QSL("@gmail.com");
  }

  msg["From"] = username.toStdString();

  auto recipients = recipientControls();
  QStringList rec_to, rec_cc, rec_bcc, rec_repl;

  for (EmailRecipientControl* ctrl : recipients) {
    switch (ctrl->recipientType()) {
      case RecipientType::Cc:
        rec_cc << ctrl->recipientAddress();
        break;

      case RecipientType::To:
        rec_to << ctrl->recipientAddress();
        break;

      case RecipientType::Bcc:
        rec_bcc << ctrl->recipientAddress();
        break;

      case RecipientType::ReplyTo:
        rec_repl << ctrl->recipientAddress();
        break;
    }
  }

  if (!rec_cc.isEmpty()) {
    msg["Cc"] = rec_cc.join(',').toStdString();
  }

  if (!rec_to.isEmpty()) {
    msg["To"] = rec_to.join(',').toStdString();
  }

  if (!rec_bcc.isEmpty()) {
    msg["Bcc"] = rec_bcc.join(',').toStdString();
  }

  if (!rec_repl.isEmpty()) {
    msg["Reply-To"] = rec_repl.join(',').toStdString();
  }

  msg["Subject"] =
    QSL("=?utf-8?B?%1?=")
      .arg(QString(m_ui.m_txtSubject->text().toUtf8().toBase64(QByteArray::Base64Option::Base64UrlEncoding)))
      .toStdString();

  // TODO: Maybe use some more advanced subclass of QTextEdit
  // to allow to change formatting etc.
  //
  // https://github.com/Anchakor/MRichTextEditor
  msg.set_html(m_ui.m_txtMessage->toHtml().toStdString());
  msg.set_header(HTTP_HEADERS_CONTENT_TYPE, "text/html; charset=utf-8");

  try {
    m_root->network()->sendEmail(msg, m_root->networkProxy(), m_originalMessage);
    accept();
  }
  catch (const ApplicationException& ex) {
    MsgBox::show(this,
                 QMessageBox::Icon::Critical,
                 tr("E-mail NOT sent"),
                 tr("Your e-mail message wasn't sent."),
                 QString(),
                 ex.message());
  }
}

EmailRecipientControl* FormAddEditEmail::addRecipientRow(const QString& recipient) {
  auto* mail_rec = new EmailRecipientControl(recipient, this);

  connect(mail_rec, &EmailRecipientControl::removalRequested, this, &FormAddEditEmail::removeRecipientRow);

  mail_rec->setPossibleRecipients(m_possibleRecipients);
  m_ui.m_layout->insertRow(m_ui.m_layout->count() - 5, mail_rec);

  return mail_rec;
}

QList<EmailRecipientControl*> FormAddEditEmail::recipientControls() const {
  QList<EmailRecipientControl*> list;

  for (int i = 0; i < m_ui.m_layout->count(); i++) {
    auto* wdg = qobject_cast<EmailRecipientControl*>(m_ui.m_layout->itemAt(i)->widget());

    if (wdg != nullptr) {
      list.append(wdg);
    }
  }

  return list;
}
