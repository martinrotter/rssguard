// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gui/formaddeditemail.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/gmail/gmailserviceroot.h"
#include "services/gmail/gui/emailrecipientcontrol.h"

FormAddEditEmail::FormAddEditEmail(GmailServiceRoot* root, QWidget* parent) : QDialog(parent), m_root(root) {
  m_ui.setupUi(this);

  m_ui.m_layoutAdder->setMargin(0);
  m_ui.m_layoutAdder->setContentsMargins(0, 0, 0, 0);

  m_ui.m_btnAdder->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_ui.m_btnAdder->setToolTip(tr("Add new recipient."));

  connect(m_ui.m_btnAdder, &PlainToolButton::clicked, this, [=]() {
    addRecipientRow();
  });
}

void FormAddEditEmail::execForAdd() {
  addRecipientRow();
  exec();
}

void FormAddEditEmail::removeRecipientRow() {
  EmailRecipientControl* sndr = static_cast<EmailRecipientControl*>(sender());

  m_ui.m_layout->takeRow(sndr);
  m_recipientControls.removeOne(sndr);

  sndr->deleteLater();
}

void FormAddEditEmail::addRecipientRow(const QString& recipient) {
  auto* mail_rec = new EmailRecipientControl(recipient, this);

  connect(mail_rec, &EmailRecipientControl::removalRequested, this, &FormAddEditEmail::removeRecipientRow);

  m_ui.m_layout->insertRow(m_ui.m_layout->indexOf(m_ui.m_txtMessage) - 1, mail_rec);
}
