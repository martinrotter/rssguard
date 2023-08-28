// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SEARCHSNODE_H
#define SEARCHSNODE_H

#include "services/abstract/rootitem.h"

#include "services/abstract/search.h"

class SearchsNode : public RootItem {
    Q_OBJECT

  public:
    explicit SearchsNode(RootItem* parent_item = nullptr);

    QList<Search*> probes() const;
    void loadProbes(const QList<Search*>& probes);

    virtual QList<Message> undeletedMessages() const;
    virtual QList<QAction*> contextMenuFeedsList();
    virtual void updateCounts(bool including_total_count);

    Search* probeById(const QString& custom_id);

  public slots:
    void createProbe();

  private:
    QAction* m_actProbeNew;
};

#endif // SEARCHSNODE_H
