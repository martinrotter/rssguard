// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STYLEDITEMDELEGATEWITHOUTFOCUS_H
#define STYLEDITEMDELEGATEWITHOUTFOCUS_H

#include <QStyledItemDelegate>
#include <QStyleOptionViewItemV4>

class StyledItemDelegateWithoutFocus : public QStyledItemDelegate {
  Q_OBJECT

  public:

    // Constructors.
    explicit StyledItemDelegateWithoutFocus(QObject* parent = 0);
    virtual ~StyledItemDelegateWithoutFocus();

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

};

#endif // STYLEDITEMDELEGATEWITHOUTFOCUS_H
