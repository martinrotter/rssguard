// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGEFILTER_H
#define MESSAGEFILTER_H

#include "core/message.h"
#include "filtering/messageobject.h"

#include <QJSEngine>
#include <QObject>

// Class which represents one message filter.
class RSSGUARD_DLLSPEC MessageFilter : public QObject {
    Q_OBJECT

  public:
    explicit MessageFilter(int id = -1, QObject* parent = nullptr);

    MessageObject::FilteringAction filterMessage(QJSEngine* engine);

    int id() const;
    void setId(int id);

    QString name() const;
    void setName(const QString& name);

    QString script() const;
    void setScript(const QString& script);

    static void initializeFilteringEngine(QJSEngine& engine, MessageObject* message_wrapper);

  private:
    int m_id;
    QString m_name;
    QString m_script;
};

#endif // MESSAGEFILTER_H
