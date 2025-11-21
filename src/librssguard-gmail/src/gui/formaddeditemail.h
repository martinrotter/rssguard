// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMADDEDITEMAIL_H
#define FORMADDEDITEMAIL_H

#include "ui_formaddeditemail.h"

#include <QDialog>

class GmailServiceRoot;
class Message;
class EmailRecipientControl;

class FormAddEditEmail : public QDialog {
    Q_OBJECT

  public:
    enum class Mode {
      SendNew,
      Reply,
      Forward
    };

    explicit FormAddEditEmail(GmailServiceRoot* root, QWidget* parent = nullptr);

  public slots:
    void show(FormAddEditEmail::Mode mode, Message* original_message = nullptr);

  private slots:
    void removeRecipientRow();
    void onOkClicked();

  private:
    EmailRecipientControl* addRecipientRow(const QString& recipient = QString());
    QString messageHeader(Mode mode, Message* original_message);
    QList<EmailRecipientControl*> recipientControls() const;

  private:
    GmailServiceRoot* m_root;

    Ui::FormAddEditEmail m_ui;
    QList<EmailRecipientControl*> m_recipientControls;
    Message* m_originalMessage;
    QStringList m_possibleRecipients;
};

#endif // FORMADDEDITEMAIL_H
