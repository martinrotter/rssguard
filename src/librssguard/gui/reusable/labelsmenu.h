// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABELSMENU_H
#define LABELSMENU_H

#include "gui/reusable/nonclosablemenu.h"
#include "services/abstract/label.h"

class LabelsMenu : public NonClosableMenu {
    Q_OBJECT

  public:
    explicit LabelsMenu(const QList<Message>& messages, const QList<Label*>& labels, QWidget* parent = nullptr);

  protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);

  signals:
    void labelsChanged();

  private slots:
    void changeLabelAssignment(Qt::CheckState state);

  private:
    void addLabelAction(Label* label, Qt::CheckState state);

  private:
    QList<Message> m_messages;
};

class LabelAction : public QAction {
    Q_OBJECT

  public:
    explicit LabelAction(Label* label, QWidget* parent_widget, QObject* parent);

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);

    Label* label() const;

  public slots:
    void toggleCheckState();

  signals:
    void checkStateChanged(Qt::CheckState state);

  private slots:
    void updateActionForState();

  private:
    Label* m_label;
    QWidget* m_parentWidget;
    Qt::CheckState m_checkState;
};

#endif // LABELSMENU_H
