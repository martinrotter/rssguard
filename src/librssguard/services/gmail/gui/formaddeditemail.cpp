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

  for (auto* rec: qAsConst(ctrls)) {
    rec->setPossibleRecipients(m_possibleRecipients);
  }
}

void FormAddEditEmail::execForAdd() {
  addRecipientRow()->setFocus();
  exec();
}

void FormAddEditEmail::execForReply(Message* original_message) {
  m_originalMessage = original_message;

  addRecipientRow(m_originalMessage->m_author);
  m_ui.m_txtSubject->setText(QSL("Re: %1").arg(m_originalMessage->m_title));
  m_ui.m_txtMessage->setFocus();
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

  msg["Subject"] = QString("=?utf-8?B?%1?=")
                   .arg(QString(m_ui.m_txtSubject->text().toUtf8().toBase64(QByteArray::Base64Option::Base64UrlEncoding)))
                   .toStdString();
  msg.set_plain(m_ui.m_txtMessage->toPlainText().toStdString());
  msg.set_header(HTTP_HEADERS_CONTENT_TYPE, "text/plain; charset=utf-8");

  try {
    m_root->network()->sendEmail(msg, m_root->networkProxy(), m_originalMessage);
    accept();
  }
  catch (const ApplicationException& ex) {
    MessageBox::show(this, QMessageBox::Icon::Critical,
                     tr("E-mail NOT sent"), tr("Your e-mail message wasn't sent."),
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
