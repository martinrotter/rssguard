// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABELSMENU_H
#define LABELSMENU_H

#include "gui/reusable/scrollablemenu.h"
#include "services/abstract/label.h"

class LabelsMenu : public ScrollableMenu {
    Q_OBJECT

  public:
    enum class Operation {
      AddLabel,
      RemoveLabel
    };

    explicit LabelsMenu(Operation operation,
                        const QList<Message>& messages,
                        const QList<Label*>& labels,
                        QWidget* parent = nullptr);

  signals:
    void labelsChanged();

  private slots:
    void changeLabelAssignment();

  private:
    QAction* labelAction(Label* label);

  private:
    QList<Message> m_messages;
    Operation m_operation;
};

class LabelAction : public QAction {
    Q_OBJECT

  public:
    explicit LabelAction(Label* label, QWidget* parent_widget, QObject* parent);

    Label* label() const;

  private:
    Label* m_label;
};

#endif // LABELSMENU_H
