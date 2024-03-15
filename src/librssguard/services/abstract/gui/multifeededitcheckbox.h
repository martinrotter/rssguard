// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MULTIFEEDEDITCHECKBOX_H
#define MULTIFEEDEDITCHECKBOX_H

#include <QCheckBox>

class RSSGUARD_DLLSPEC MultiFeedEditCheckBox : public QCheckBox {
    Q_OBJECT

  public:
    explicit MultiFeedEditCheckBox(QWidget* parent = nullptr);

    QList<QWidget*> actionWidgets() const;
    void addActionWidget(QWidget* widget);

  private:
    QList<QWidget*> m_actionWidgets;
};

#endif // MULTIFEEDEDITCHECKBOX_H
