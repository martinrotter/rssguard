// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABELSNODE_H
#define LABELSNODE_H

#include "services/abstract/rootitem.h"

#include "services/abstract/label.h"

class LabelsNode : public RootItem {
  Q_OBJECT

  public:
    explicit LabelsNode(const QList<Label*>& labels, RootItem* parent_item = nullptr);

    virtual QList<QAction*> contextMenuFeedsList();

  public slots:
    void createLabel();

  private:
    QAction* m_actLabelNew;
};

#endif // LABELSNODE_H
