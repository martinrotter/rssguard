// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STYLEDITEMDELEGATEWITHOUTFOCUS_H
#define STYLEDITEMDELEGATEWITHOUTFOCUS_H

#include <QStyledItemDelegate>

#if QT_VERSION_MAJOR <= 5
#include <QStyleOptionViewItemV4>
#else
#include <QStyleOptionViewItem>
#endif

class StyledItemDelegateWithoutFocus : public QStyledItemDelegate {
  Q_OBJECT

  public:
    explicit StyledItemDelegateWithoutFocus(QObject* parent = nullptr);

    QSize sizeHint ( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

};

inline QSize StyledItemDelegateWithoutFocus::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  QSize siz = QStyledItemDelegate::sizeHint(option, index);

  /*   QStyleOptionViewItem opt = option;

         initStyleOption(&opt, index);
         QStyle* style = widget ? widget->style() : QApplication::style();

         return style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), widget);*/

  return siz;
}

#endif // STYLEDITEMDELEGATEWITHOUTFOCUS_H
