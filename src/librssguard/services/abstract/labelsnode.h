// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABELSNODE_H
#define LABELSNODE_H

#include "services/abstract/label.h"
#include "services/abstract/rootitem.h"

class RSSGUARD_DLLSPEC LabelsNode : public RootItem {
    Q_OBJECT

  public:
    explicit LabelsNode(RootItem* parent_item = nullptr);

    QList<Label*> labels() const;
    void loadLabels(const QList<Label*>& labels);

    virtual void markAsReadUnread(RootItem::ReadStatus status);
    virtual QString additionalTooltip() const;
    virtual QList<QAction*> contextMenuFeedsList();
    virtual void updateCounts(bool including_total_count);

    Label* labelByCustomId(const QString& custom_id);
    QHash<QString, Label*> getHashedLabels() const;

  public slots:
    void createLabel();

  private:
    QAction* m_actLabelNew;
};

#endif // LABELSNODE_H
