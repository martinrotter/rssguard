// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMADDEDITEMAIL_H
#define FORMADDEDITEMAIL_H

#include "ui_formaddeditemail.h"

#include <QDialog>

namespace Ui {
  class FormAddEditEmail;
}

class GmailServiceRoot;
class Message;
class EmailRecipientControl;

class FormAddEditEmail : public QDialog {
    Q_OBJECT

  public:
    explicit FormAddEditEmail(GmailServiceRoot* root, QWidget* parent = nullptr);

  public slots:
    void execForAdd();
    void execForReply(Message* original_message);
    void execForForward(Message* original_message);

  private slots:
    void removeRecipientRow();
    void onOkClicked();
    EmailRecipientControl* addRecipientRow(const QString& recipient = QString());

  private:
    QList<EmailRecipientControl*> recipientControls() const;

  private:
    GmailServiceRoot* m_root;

    Ui::FormAddEditEmail m_ui;
    QList<EmailRecipientControl*> m_recipientControls;
    Message* m_originalMessage;
    QStringList m_possibleRecipients;
};

#endif // FORMADDEDITEMAIL_H
