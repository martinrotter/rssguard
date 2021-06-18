// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/labelsnode.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "gui/dialogs/formaddeditlabel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

#include "3rd-party/boolinq/boolinq.h"

LabelsNode::LabelsNode(RootItem* parent_item) : RootItem(parent_item), m_actLabelNew(nullptr) {
  setKind(RootItem::Kind::Labels);
  setId(ID_LABELS);
  setIcon(qApp->icons()->fromTheme(QSL("tag-folder")));
  setTitle(tr("Labels"));
  setDescription(tr("You can see all your labels (tags) here."));
}

void LabelsNode::loadLabels(const QList<Label*>& labels) {
  for (Label* lbl : labels) {
    appendChild(lbl);
  }
}

QList<Message> LabelsNode::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedLabelledMessages(database, getParentServiceRoot()->accountId());
}

QList<Label*> LabelsNode::labels() const {
  auto list = boolinq::from(childItems()).select([](RootItem* it) {
    return static_cast<Label*>(it);
  }).toStdList();

  return FROM_STD_LIST(QList<Label*>, list);
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
  if ((getParentServiceRoot()->supportedLabelOperations() & ServiceRoot::LabelOperation::Adding) ==
      ServiceRoot::LabelOperation::Adding) {
    FormAddEditLabel frm(qApp->mainFormWidget());
    Label* new_lbl = frm.execForAdd();

    if (new_lbl != nullptr) {
      QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

      DatabaseQueries::createLabel(db, new_lbl, getParentServiceRoot()->accountId());

      getParentServiceRoot()->requestItemReassignment(new_lbl, this);
    }
  }
  else {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         tr("This account does not allow you to create labels."),
                         tr("Not allowed"),
                         QSystemTrayIcon::MessageIcon::Critical,
                         true);
  }
}
