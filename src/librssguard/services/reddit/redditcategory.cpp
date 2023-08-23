// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/reddit/redditcategory.h"

RedditCategory::RedditCategory(Type type, RootItem* parent_item)
  : Category(parent_item), m_type(type) {
  updateTitle();
}

RedditCategory::Type RedditCategory::type() const {
  return m_type;
}

void RedditCategory::updateTitle() {
  switch (m_type) {
    case Type::Subscriptions:
      setTitle(tr("Subscriptions"));
      break;
  }
}
