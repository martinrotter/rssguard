// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTERINGSYSTEM_H
#define FILTERINGSYSTEM_H

#include "filtering/filterobjects.h"
#include "filtering/messagefilter.h"

#include <QJSEngine>
#include <QObject>

class FilteringSystem : public QObject {
    Q_OBJECT

  public:
    enum class FiteringUseCase {
      NewArticles,
      ExistingArticles
    };

    explicit FilteringSystem(FiteringUseCase mode, Feed* feed, ServiceRoot* account, QObject* parent = nullptr);

    void setMessage(Message* message);

    void pushMessageStatesToServices(QList<Message>& read_msgs,
                                     QList<Message>& important_msgs,
                                     RootItem* item,
                                     ServiceRoot* account);
    void compareAndWriteArticleStates(Message* msg_original,
                                      Message* msg_filtered,
                                      QList<Message>& read_msgs,
                                      QList<Message>& important_msgs);

    FilterMessage::FilteringAction filterMessage(const MessageFilter& filter);

    QJSEngine& engine();
    FilterMessage& message();
    Feed* feed() const;
    ServiceRoot* account() const;
    QList<Label*>& availableLabels();
    FiteringUseCase mode() const;
    FilterRun& filterRun();
    FilterAccount& filterAccount();
    FilterApp& filterApp();

  private:
    void initializeEngine();

  private:
    FiteringUseCase m_mode;
    Feed* m_feed;
    ServiceRoot* m_account;
    QList<Label*> m_availableLabels;

    QJSEngine m_engine;

    FilterMessage m_filterMessage;
    FilterFeed m_filterFeed;
    FilterUtils m_filterUtils;
    FilterApp m_filterApp;
    FilterAccount m_filterAccount;
    FilterRun m_filterRun;
    FilterFs m_filterFs;
};

#endif // FILTERINGSYSTEM_H
