// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTERINGSYSTEM_H
#define FILTERINGSYSTEM_H

#include "filtering/filterobjects.h"
#include "filtering/messagefilter.h"

#include <QDateTime>
#include <QHash>
#include <QJSEngine>
#include <QObject>

class ApplicationPaths;
class DatabaseFactory;
class GuiNotificationCoordinator;
class IconFactory;
class Localization;

class FilteringSystem : public QObject {
    Q_OBJECT

  public:
    enum class FiteringUseCase {
      NewArticles,
      ExistingArticles
    };

    struct DuplicateCandidate {
        int m_id;
        QString m_title;
        QString m_url;
        QString m_author;
        QDateTime m_created;
        QString m_customId;
    };

    explicit FilteringSystem(FiteringUseCase mode,
                             Feed* feed,
                             ServiceRoot* account,
                             ApplicationPaths* app,
                             DatabaseFactory* db_factory,
                             IconFactory* icon_factory,
                             Localization* localization,
                             GuiNotificationCoordinator* gui_notif,
                             QObject* parent = nullptr);

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

    const QList<DuplicateCandidate>& duplicateCandidates(FilterMessage::DuplicityCheck criteria);
    void removeDuplicateCandidate(int message_id);

    QJSEngine& engine();
    FilterMessage& message();
    Feed* feed() const;
    ServiceRoot* account() const;
    QList<Label*>& availableLabels();
    FiteringUseCase mode() const;
    FilterRun& filterRun();
    FilterAccount& filterAccount();
    FilterApp& filterApp();
    ApplicationPaths* applicationPaths() const;
    DatabaseFactory* database() const;
    IconFactory* icons() const;
    Localization* localization() const;
    GuiNotificationCoordinator* guiNotifications() const;

  private:
    void initializeEngine();
    QJSValue prepareFilter(const MessageFilter& filter);

  private:
    FiteringUseCase m_mode;
    Feed* m_feed;
    ServiceRoot* m_account;
    QList<Label*> m_availableLabels;
    ApplicationPaths* m_applicationPaths;
    DatabaseFactory* m_databaseFactory;
    IconFactory* m_iconFactory;
    Localization* m_localization;
    GuiNotificationCoordinator* m_guiNotifications;

    QJSEngine m_engine;
    QHash<const MessageFilter*, QJSValue> m_preparedFilters;
    QList<DuplicateCandidate> m_accountDuplicateCandidates;
    QList<DuplicateCandidate> m_feedDuplicateCandidates;
    bool m_accountDuplicateCandidatesLoaded{false};
    bool m_feedDuplicateCandidatesLoaded{false};

    FilterMessage m_filterMessage;
    FilterFeed m_filterFeed;
    FilterUtils m_filterUtils;
    FilterApp m_filterApp;
    FilterAccount m_filterAccount;
    FilterRun m_filterRun;
    FilterFs m_filterFs;
};

#endif // FILTERINGSYSTEM_H
