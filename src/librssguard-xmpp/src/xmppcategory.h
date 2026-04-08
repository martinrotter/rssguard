// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef XMPPCATEGORY_H
#define XMPPCATEGORY_H

#include <librssguard/services/abstract/category.h>

class XmppCategory : public Category {
    Q_OBJECT

  public:
    enum Type {
      Unknown = 0,
      SingleUserChats = 1,
      MultiUserChats = 2,
      PubSubServices = 3,
      PubSubPeps = 4
    };

    explicit XmppCategory(Type type = Type::SingleUserChats, RootItem* parent = nullptr);
    explicit XmppCategory(const XmppCategory& other);

    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);

    Type type() const;
    void setType(Type type);

    static QString typeToString(Type type);

  private:
    void updateTitleAndIcon();

  private:
    Type m_type;
};

#endif // XMPPCATEGORY_H
