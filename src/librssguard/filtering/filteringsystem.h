// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTERINGSYSTEM_H
#define FILTERINGSYSTEM_H

#include "filtering/filterobjects.h"
#include "filtering/messagefilter.h"
#include "filtering/messageobject.h"

#include <QJSEngine>
#include <QObject>
#include <QSqlDatabase>

class FilteringSystem : public QObject {
    Q_OBJECT

  public:
    enum class FiteringUseCase {
      NewArticles,
      ExistingArticles
    };

    explicit FilteringSystem(FiteringUseCase mode,
                             QSqlDatabase& db,
                             Feed* feed,
                             ServiceRoot* account,
                             QObject* parent = nullptr);

    void setMessage(Message* message);

    MessageObject::FilteringAction filterMessage(const MessageFilter& filter);

    QJSEngine& engine();
    MessageObject& message();
    QSqlDatabase& database();
    Feed* feed() const;
    ServiceRoot* account() const;
    QList<Label*>& availableLabels();
    FiteringUseCase mode() const;

  private:
    void initializeEngine();

  private:
    FiteringUseCase m_mode;
    QSqlDatabase m_db;
    Feed* m_feed;
    ServiceRoot* m_account;
    QList<Label*> m_availableLabels;

    QJSEngine m_engine;
    MessageObject m_message;
    FilterUtils m_filterUtils;
    FilterApp m_filterApp;
    FilterRun m_filterRun;
};

#endif // FILTERINGSYSTEM_H
