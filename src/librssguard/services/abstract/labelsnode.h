// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABELSNODE_H
#define LABELSNODE_H

#include "services/abstract/rootitem.h"

#include "services/abstract/label.h"

class LabelsNode : public RootItem {
    Q_OBJECT

  public:
    explicit LabelsNode(RootItem* parent_item = nullptr);

    QList<Label*> labels() const;
    void loadLabels(const QList<Label*>& labels);

    virtual QList<Message> undeletedMessages() const;
    virtual QList<QAction*> contextMenuFeedsList();
    virtual int countOfUnreadMessages() const;
    virtual int countOfAllMessages() const;

  public slots:
    void createLabel();

  private:
    QAction* m_actLabelNew;
};

#endif // LABELSNODE_H
