// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/labelsnode.h"

#include "gui/dialogs/formaddeditlabel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

LabelsNode::LabelsNode(RootItem* parent_item) : RootItem(parent_item), m_actLabelNew(nullptr) {
  setKind(RootItem::Kind::Labels);
  setId(ID_LABELS);
  setIcon(qApp->icons()->fromTheme(QSL("tag-folder")));
  setTitle(tr("Labels"));
  setDescription(tr("You can see all your labels (tags) here."));
  setCreationDate(QDateTime::currentDateTime());
}

void LabelsNode::loadLabels(const QList<Label*>& labels) {
  for (Label* lbl : labels) {
    appendChild(lbl);
  }
}

QList<QAction*> LabelsNode::contextMenuFeedsList() {
  if (m_actLabelNew == nullptr) {
    // Initialize it all.
    m_actLabelNew = new QAction(qApp->icons()->fromTheme(QSL("tag-new")), tr("New label"), this);

    connect(m_actLabelNew, &QAction::triggered, this, &LabelsNode::createLabel);
  }

  return QList<QAction*> {
    m_actLabelNew
  };
}

void LabelsNode::createLabel() {
  FormAddEditLabel frm(qApp->mainFormWidget());
  Label* new_lbl = frm.execForAdd();

  if (new_lbl != nullptr) {
    QSqlDatabase db = qApp->database()->connection(metaObject()->className());

    DatabaseQueries::createLabel(db, new_lbl, getParentServiceRoot()->accountId());

    getParentServiceRoot()->requestItemReassignment(new_lbl, this);
  }
}
