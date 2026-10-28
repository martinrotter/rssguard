// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESFORFILTERSMODEL_H
#define MESSAGESFORFILTERSMODEL_H

#include "database/messageupdateresult.h"
#include "filtering/filterobjects.h"

#include <memory>
#include <optional>

#include <QAbstractTableModel>

// Indexes of columns for message filter manager models.
#define MFM_MODEL_RESULT      0
#define MFM_MODEL_DATABASE    1
#define MFM_MODEL_ISREAD      2
#define MFM_MODEL_ISIMPORTANT 3
#define MFM_MODEL_ISDELETED   4
#define MFM_MODEL_TITLE       5
#define MFM_MODEL_CREATED     6
#define MFM_MODEL_RETRIEVED   7
#define MFM_MODEL_SCORE       8

#define VALUE_COMPARISON_FORMAT "%2  ↴\n%1"

class MessageFilter;
class FilteringSystem;
class Application;

struct MessageBackupAndOriginal {
    Message m_original;
    Message m_filtered;
    Message m_expectedStored;
    std::optional<FilterMessage::FilteringAction> m_decision;
    std::optional<MessageWriteResult> m_saveResult;
    QString m_error;
    bool m_purged = false;
    bool m_compact = false;
    bool m_hasStoredValues = true;
};

class MessagesForFiltersModel : public QAbstractTableModel {
    Q_OBJECT

  public:
    explicit MessagesForFiltersModel(Application* app, QObject* parent = nullptr);

    virtual int rowCount(const QModelIndex& parent) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;

  public:
    void processFeeds(MessageFilter* fltr, ServiceRoot* account, const QList<RootItem*>& checked);
    void testFilter(MessageFilter* filter, ServiceRoot* account);

    // Enclosures are mutable QObjects, so ordinary Message copies are not snapshots.
    static Message snapshotForFiltering(const Message& message);
    void setShowStoredValues(bool show_stored);
    QString summaryText() const;
    QString lastError() const;
    bool isPreview() const;
    bool hasStoredValuesForRow(int row) const;

    Message* messageForRow(int row);
    const Message* constMessageForRow(int row) const;
    const MessageBackupAndOriginal* resultForRow(int row) const;
    Message messageDetailsForRow(int row) const;

  public slots:
    void setMessages(const QList<Message>& messages);

  signals:
    void scriptLogged(const QString& text);
    void summaryChanged(const QString& text);
    void resultsChanged(bool preview);

  private:
    QString decisionToText(FilterMessage::FilteringAction dec) const;
    QString databaseOutcome(const MessageBackupAndOriginal& row, bool detailed) const;
    std::unique_ptr<FilteringSystem> createFilteringSystem(Feed* feed, ServiceRoot* account);
    std::optional<Message> storedMessage(int id) const;
    QHash<int, Message> storedMessages(const QList<int>& ids, bool full) const;
    void refreshStoredRows(int first_row, int count);
    void finishRun(int completed_feeds, int requested_feeds);

  private:
    Application* m_application;
    QList<QString> m_headerData{};
    QList<MessageBackupAndOriginal> m_messages{};

    ServiceRoot* m_resultAccount = nullptr;
    QString m_summary;
    QString m_error;
    bool m_preview = false;
    bool m_showStoredValues = false;

    QColor m_colorOk;
    QColor m_colorError;
    QString m_txtTrue;
    QString m_txtFalse;
};

#endif // MESSAGESFORFILTERSMODEL_H
