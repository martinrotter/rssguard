// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABEL_H
#define LABEL_H

#include "services/abstract/rootitem.h"

#include <QColor>

class Label : public RootItem {
  Q_OBJECT

  public:
    explicit Label(const QString& name, const QColor& color, RootItem* parent_item = nullptr);
    explicit Label(RootItem* parent_item = nullptr);

    QColor color() const;
    void setColor(const QColor& color);

    virtual bool canBeEdited() const;
    virtual bool editViaGui();
    virtual bool canBeDeleted() const;
    virtual bool deleteViaGui();
    static QIcon generateIcon(const QColor& color);

  private:
    QColor m_color;
};

#endif // LABEL_H
