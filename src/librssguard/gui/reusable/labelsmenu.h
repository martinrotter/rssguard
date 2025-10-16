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
      RemoveLabel,
      Toggle
    };

    explicit LabelsMenu(Operation operation, QWidget* parent = nullptr);

    QList<Message> messages() const;
    void setMessages(const QList<Message>& messages);

    void setLabels(const QList<Label*>& labels);

    QList<QAction*> labelActions() const;

  signals:
    void setModelArticleLabelIds(int article_id, const QStringList& label_custom_ids);

  private slots:
    void changeLabelAssignment(bool assign);

  private:
    QAction* labelAction(Label* label);

  private:
    QList<Message> m_messages;
    Operation m_operation;
    QList<QAction*> m_labelActions;
};

class LabelAction : public QAction {
    Q_OBJECT

  public:
    explicit LabelAction(Label* label, QObject* parent);

    Label* label() const;

  private:
    Label* m_label;
};

#endif // LABELSMENU_H
