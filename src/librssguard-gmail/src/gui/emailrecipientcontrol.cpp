// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/emailrecipientcontrol.h"

#include "src/definitions.h"

#include <librssguard/gui/reusable/plaintoolbutton.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

#include <QComboBox>
#include <QCompleter>
#include <QHBoxLayout>
#include <QLineEdit>

EmailRecipientControl::EmailRecipientControl(const QString& recipient, QWidget* parent)
  : QWidget(parent), m_possibleRecipientsModel(nullptr) {
  auto* lay = new QHBoxLayout(this);

  lay->addWidget(m_cmbRecipientType = new QComboBox(this));
  lay->addWidget(m_txtRecipient = new QLineEdit(this), 1);
  lay->addWidget(m_btnCloseMe = new PlainToolButton(this));

  lay->setContentsMargins({});

  m_cmbRecipientType->setFocusPolicy(Qt::FocusPolicy::NoFocus);
  m_btnCloseMe->setFocusPolicy(Qt::FocusPolicy::NoFocus);
  m_txtRecipient->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_txtRecipient->setPlaceholderText(tr("E-mail address"));
  m_txtRecipient->setText(recipient);

  setFocusProxy(m_txtRecipient);

  m_btnCloseMe->setToolTip(QSL("Remove this recipient."));
  m_btnCloseMe->setIcon(qApp->icons()->fromTheme(QSL("list-remove")));

  connect(m_btnCloseMe, &PlainToolButton::clicked, this, &EmailRecipientControl::removalRequested);
  connect(m_txtRecipient, &QLineEdit::textEdited, this, [this]() {
    m_recipientMessageCustomId.clear();
  });

  m_cmbRecipientType->addItem(tr("To"), int(RecipientType::To));
  m_cmbRecipientType->addItem(tr("Cc"), int(RecipientType::Cc));
  m_cmbRecipientType->addItem(tr("Bcc"), int(RecipientType::Bcc));
  m_cmbRecipientType->addItem(tr("Reply-to"), int(RecipientType::ReplyTo));

  setTabOrder(m_cmbRecipientType, m_txtRecipient);
  setTabOrder(m_txtRecipient, m_btnCloseMe);

  setLayout(lay);
}

QString EmailRecipientControl::recipientAddress() const {
  return m_txtRecipient->text();
}

QString EmailRecipientControl::recipientMessageCustomId() const {
  if (!m_recipientMessageCustomId.isEmpty() || m_possibleRecipientsModel == nullptr) {
    return m_recipientMessageCustomId;
  }

  const QString current_text = m_txtRecipient->text();

  for (int row = 0; row < m_possibleRecipientsModel->rowCount(); ++row) {
    const QModelIndex idx = m_possibleRecipientsModel->index(row, 0);

    if (idx.data(Qt::ItemDataRole::DisplayRole).toString() == current_text) {
      return idx.data(MessageCustomIdRole).toString();
    }
  }

  return {};
}

void EmailRecipientControl::setRecipientAddress(const QString& recipient) {
  m_txtRecipient->setText(recipient);
}

RecipientType EmailRecipientControl::recipientType() const {
  return RecipientType(m_cmbRecipientType->currentData(Qt::ItemDataRole::UserRole).toInt());
}

void EmailRecipientControl::setPossibleRecipientsModel(QAbstractItemModel* model) {
  m_possibleRecipientsModel = model;

  if (m_txtRecipient->completer() != nullptr) {
    auto* old_cmpl = m_txtRecipient->completer();

    m_txtRecipient->setCompleter(nullptr);
    old_cmpl->deleteLater();
  }

  if (model == nullptr) {
    return;
  }

  QCompleter* cmpl = new QCompleter(model, m_txtRecipient);

  cmpl->setFilterMode(Qt::MatchFlag::MatchContains);
  cmpl->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
  cmpl->setCompletionMode(QCompleter::CompletionMode::UnfilteredPopupCompletion);

  connect(cmpl, qOverload<const QModelIndex&>(&QCompleter::activated), this, [this](const QModelIndex& idx) {
    m_recipientMessageCustomId = idx.data(MessageCustomIdRole).toString();

    if (!m_recipientMessageCustomId.isEmpty()) {
      emit recipientSelected(m_recipientMessageCustomId, idx.data(Qt::ItemDataRole::DisplayRole).toString());
    }
  });

  m_txtRecipient->setCompleter(cmpl);
}
