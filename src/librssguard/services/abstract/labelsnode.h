// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABELSNODE_H
#define LABELSNODE_H

#include "services/abstract/rootitem.h"

class LabelsNode : public RootItem {
  Q_OBJECT

  public:
    explicit LabelsNode(RootItem* parent_item = nullptr);
};

#endif // LABELSNODE_H
