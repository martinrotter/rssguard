// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef EMAILRECIPIENTCONTROL_H
#define EMAILRECIPIENTCONTROL_H

#include "src/definitions.h"

#include <QAbstractItemModel>
#include <QPointer>
#include <QWidget>

class QComboBox;
class QLineEdit;
class PlainToolButton;

class EmailRecipientControl : public QWidget {
    Q_OBJECT

  public:
    static constexpr int MessageCustomIdRole = Qt::ItemDataRole::UserRole + 1;

    explicit EmailRecipientControl(const QString& recipient, QWidget* parent = nullptr);

  public:
    QString recipientAddress() const;
    QString recipientMessageCustomId() const;
    RecipientType recipientType() const;

    void setRecipientAddress(const QString& recipient);
    void setPossibleRecipientsModel(QAbstractItemModel* model);

  signals:
    void removalRequested();
    void recipientSelected(const QString& message_custom_id, const QString& recipient);

  private:
    QComboBox* m_cmbRecipientType;
    QLineEdit* m_txtRecipient;
    PlainToolButton* m_btnCloseMe;
    QPointer<QAbstractItemModel> m_possibleRecipientsModel;
    QString m_recipientMessageCustomId;
};

#endif // EMAILRECIPIENTCONTROL_H
