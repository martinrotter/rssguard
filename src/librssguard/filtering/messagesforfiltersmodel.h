// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESFORFILTERSMODEL_H
#define MESSAGESFORFILTERSMODEL_H

#include "filtering/filterobjects.h"

#include <QAbstractTableModel>

class MessageFilter;
class FilteringSystem;

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
    int messagesCount() const;

    void processFeeds(MessageFilter* fltr, ServiceRoot* account, const QList<RootItem*>& checked);
    void testFilter(MessageFilter* filter, FilteringSystem* engine);

    Message messageForRow(int row) const;
    Message* messageForRow(int row);

  public slots:
    void setMessages(const QList<Message>& messages);

  private:
    QList<QString> m_headerData{};
    QList<Message> m_messages{};

    // Key is integer position of the message within the list of messages.
    QMap<int, FilterMessage::FilteringAction> m_filteringDecisions{};
};

#endif // MESSAGESFORFILTERSMODEL_H
