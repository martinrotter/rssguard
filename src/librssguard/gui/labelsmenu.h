// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABELSMENU_H
#define LABELSMENU_H

#include <QMenu>
#include <QWidgetAction>

#include "services/abstract/label.h"

class QCheckBox;

class LabelsMenu : public QMenu {
  Q_OBJECT

  public:
    explicit LabelsMenu(const QList<Message>& messages, const QList<Label*>& labels, QWidget* parent = nullptr);

  protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);

  private:
    void addLabelAction(const Label* label, Qt::CheckState state);
};

class LabelAction : public QAction {
  Q_OBJECT

  public:
    explicit LabelAction(const Label* label, QWidget* parent_widget, QObject* parent);

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);

    const Label* label() const;

  public slots:
    void toggle();

  signals:
    void checkStateChanged(Qt::CheckState state);

  private slots:
    void updateActionForState();

  private:
    const Label* m_label;
    QWidget* m_parentWidget;
    Qt::CheckState m_checkState;
};

#endif // LABELSMENU_H
