// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STYLEDITEMDELEGATE_H
#define STYLEDITEMDELEGATE_H

#include <QStyledItemDelegate>

#if QT_VERSION_MAJOR <= 5
#include <QStyleOptionViewItemV4>
#else
#include <QStyleOptionViewItem>
#endif

class QTreeView;

class StyledItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
    Q_PROPERTY(qreal flashProgress READ flashProgress WRITE setFlashProgress NOTIFY flashProgressChanged)

  public:
    explicit StyledItemDelegate(int height_row, int padding_row, QObject* parent = nullptr);

    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

    void flashItem(const QModelIndex& index, QTreeView* view);

    qreal flashProgress() const;
    void setFlashProgress(qreal v);

  signals:
    void flashProgressChanged();

  private:
    QRect rowRectForIndex(QTreeView* view, const QModelIndex& idx) const;

    QColor m_flashColor;
    QModelIndex m_flashIndex;
    qreal m_flashProgress;
    int m_rowHeight;
    int m_rowPadding;
};

#endif // STYLEDITEMDELEGATE_H
