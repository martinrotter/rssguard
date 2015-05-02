#ifndef FEEDSSELECTION_H
#define FEEDSSELECTION_H

#include <QString>
#include <QObject>


class FeedsModelRootItem;

class FeedsSelection {
  public:
    enum MessageMode {
      NoMode,
      MessagesFromFeeds,
      MessagesFromRecycleBin
    };

    explicit FeedsSelection(FeedsModelRootItem *root_of_selection = NULL);
    FeedsSelection(const FeedsSelection &other);
    virtual ~FeedsSelection();

  MessageMode mode();
    FeedsModelRootItem *selectedItem() const;
    QString generateDatabaseFilter();

  private:
    FeedsModelRootItem *m_selectedItem;
};

Q_DECLARE_METATYPE(FeedsSelection::MessageMode)

#endif // FEEDSSELECTION_H
