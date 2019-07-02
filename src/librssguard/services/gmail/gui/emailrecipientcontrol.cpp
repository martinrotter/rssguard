// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gui/emailrecipientcontrol.h"

#include "gui/plaintoolbutton.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/gmail/definitions.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>

EmailRecipientControl::EmailRecipientControl(const QString& recipient, QWidget* parent) : QWidget(parent) {
  QHBoxLayout* lay = new QHBoxLayout(this);

  lay->addWidget(m_cmbRecipientType = new QComboBox(this));
  lay->addWidget(m_txtRecipient = new QLineEdit(this), 1);
  lay->addWidget(m_btnCloseMe = new PlainToolButton(this));
  lay->setMargin(0);
  lay->setContentsMargins(0, 0, 0, 0);

  m_btnCloseMe->setToolTip("Remove this recipient.");
  m_btnCloseMe->setIcon(qApp->icons()->fromTheme(QSL("list-remove")));

  connect(m_btnCloseMe, &PlainToolButton::clicked, this, &EmailRecipientControl::removalRequested);

  m_cmbRecipientType->addItem(tr("To"), int(RecipientType::To));
  m_cmbRecipientType->addItem(tr("Cc"), int(RecipientType::Cc));
  m_cmbRecipientType->addItem(tr("Bcc"), int(RecipientType::Bcc));
  m_cmbRecipientType->addItem(tr("Reply-to"), int(RecipientType::ReplyTo));

  setTabOrder(m_cmbRecipientType, m_txtRecipient);
  setTabOrder(m_txtRecipient, m_btnCloseMe);

  setLayout(lay);
}
