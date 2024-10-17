// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STYLEDITEMDELEGATEWITHOUTFOCUS_H
#define STYLEDITEMDELEGATEWITHOUTFOCUS_H

#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class StyledItemDelegateWithoutFocus : public QStyledItemDelegate {
    Q_OBJECT

  public:
    explicit StyledItemDelegateWithoutFocus(int height_row, int padding_row, QObject* parent = nullptr);

    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

  private:
    int m_rowHeight;
    int m_rowPadding;
};

#endif // STYLEDITEMDELEGATEWITHOUTFOCUS_H
