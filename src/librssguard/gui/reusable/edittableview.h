// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef EDITTABLEVIEW_H
#define EDITTABLEVIEW_H

#include <QTableView>

class EditTableView : public QTableView {
  Q_OBJECT

  public:
    explicit EditTableView(QWidget* parent = nullptr);

  public slots:
    void removeSelected();
    void removeAll();

  private:
    void keyPressEvent(QKeyEvent* event);
};

#endif // EDITTABLEVIEW_H
