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
    explicit StyledItemDelegateWithoutFocus(const QString& row_height_settings_key, QObject* parent = nullptr);

    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

  private:
    QString m_rowHeightSettingsKey;
};

#endif // STYLEDITEMDELEGATEWITHOUTFOCUS_H
