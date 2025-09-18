// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESFORFILTERSMODEL_H
#define MESSAGESFORFILTERSMODEL_H

#include "filtering/filterobjects.h"

#include <QAbstractTableModel>

// Indexes of columns for message filter manager models.
#define MFM_MODEL_RESULT      0
#define MFM_MODEL_ISREAD      1
#define MFM_MODEL_ISIMPORTANT 2
#define MFM_MODEL_ISDELETED   3
#define MFM_MODEL_TITLE       4
#define MFM_MODEL_CREATED     5
#define MFM_MODEL_SCORE       6

class MessageFilter;
class FilteringSystem;

struct MessageBackupAndOriginal {
    Message m_original;
    Message m_filtered;
};

class MessagesForFiltersModel : public QAbstractTableModel {
    Q_OBJECT

  public:
    explicit MessagesForFiltersModel(QObject* parent = nullptr);

    virtual int rowCount(const QModelIndex& parent) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;

  public:
    void processFeeds(MessageFilter* fltr, ServiceRoot* account, const QList<RootItem*>& checked);
    void testFilter(MessageFilter* filter, FilteringSystem* engine);

    Message messageForRow(int row) const;
    Message* messageForRow(int row);

  public slots:
    void setMessages(const QList<Message>& messages);

  private:
    QString decisionToText(FilterMessage::FilteringAction dec) const;

  private:
    QList<QString> m_headerData{};
    QList<MessageBackupAndOriginal> m_messages{};

    // Key is integer position of the message within the list of messages.
    QMap<int, FilterMessage::FilteringAction> m_filteringDecisions{};
};

#endif // MESSAGESFORFILTERSMODEL_H
