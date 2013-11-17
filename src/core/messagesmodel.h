#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include <QAbstractItemModel>
#include <QFont>


// Representation of ONE message.
class Message {
  private:
    QList<QVariant> m_data;
/*
    int m_id;
    int m_read;
    int m_deleted;
    int m_important;
    QString m_title;
    QString m_url;
    QString m_author;
    QString m_dateCreated;
    QString m_dateUpdate;
    QString m_contents;
*/
    friend class MessagesModel;
};

class MessagesModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesModel(QObject *parent = 0);
    virtual ~MessagesModel();

    // Model implementation.

    // Data accessors/manipulators.
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    bool setData(int row, int column, const QVariant &value);
    Qt::ItemFlags flags(const QModelIndex &index) const;

    // Model dimensions.
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    // Model navigation.
    QModelIndex parent(const QModelIndex &child) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

  public slots:
    // Sets "read" status of message with given row index.
    void setMessageRead(int row_index, int read);

  private:
    void setupHeaderData();

    // Creates "normal" and "bold" fonts.
    void setupFonts();

  private:
    QHash<int, int> m_columnMappings;
    QList<Message> m_messages;

    QFont m_normalFont;
    QFont m_boldFont;
    QList<QString> m_headerData;

};

#endif // MESSAGESMODEL_H
