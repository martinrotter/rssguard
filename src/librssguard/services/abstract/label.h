// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABEL_H
#define LABEL_H

#include "services/abstract/rootitem.h"

#include <QColor>

class Label : public RootItem {
  public:
    explicit Label(RootItem* parent_item = nullptr);

  private:
    QString m_name;
    QColor m_backColor;
};

#endif // LABEL_H
