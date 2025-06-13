// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STYLEDITEMDELEGATE_H
#define STYLEDITEMDELEGATE_H

#include <QStyledItemDelegate>

#if QT_VERSION_MAJOR <= 5
#include <QStyleOptionViewItemV4>
#else
#include <QStyleOptionViewItem>
#endif

class StyledItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    explicit StyledItemDelegate(int height_row, int padding_row, QObject* parent = nullptr);

    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

  private:
    int m_rowHeight;
    int m_rowPadding;
};

#endif // STYLEDITEMDELEGATE_H
