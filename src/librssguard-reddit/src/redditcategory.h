// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef REDDITCATEGORY_H
#define REDDITCATEGORY_H

#include "services/abstract/category.h"

class RedditCategory : public Category {
  Q_OBJECT

  public:
    enum class Type {
      Subscriptions = 1
    };

    explicit RedditCategory(Type type = Type::Subscriptions, RootItem* parent_item = nullptr);

    Type type() const;

  private:
    void updateTitle();

  private:
    Type m_type;
};

#endif // REDDITCATEGORY_H
