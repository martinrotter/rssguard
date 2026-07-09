// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMADDEDITEMAIL_H
#define FORMADDEDITEMAIL_H

#include "ui_formaddeditemail.h"

#include <QDialog>
#include <QStandardItemModel>

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
    QString recipientAddress(EmailRecipientControl* ctrl);
    void resolveRecipientFromMetadata(EmailRecipientControl* ctrl,
                                      const QString& message_custom_id,
                                      const QString& fallback_recipient);
    void updateRecipientInCompleterModel(const QString& message_custom_id, const QString& recipient);

  private:
    GmailServiceRoot* m_root;
    Ui::FormAddEditEmail m_ui;
    QList<EmailRecipientControl*> m_recipientControls;
    Message* m_originalMessage;
    QStandardItemModel m_possibleRecipientsModel;
};

#endif // FORMADDEDITEMAIL_H
