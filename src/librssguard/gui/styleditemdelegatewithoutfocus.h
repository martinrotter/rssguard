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

    QSize sizeHint ( const QStyleOptionViewItem& option, const QModelIndex& index ) const
    {
      QSize siz = QStyledItemDelegate::sizeHint(option, index);

      /*   QStyleOptionViewItem opt = option;

         initStyleOption(&opt, index);
         QStyle* style = widget ? widget->style() : QApplication::style();

         return style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), widget);*/

      return siz;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

};

#endif // STYLEDITEMDELEGATEWITHOUTFOCUS_H
