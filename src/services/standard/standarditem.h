#ifndef STANDARDITEM_H
#define STANDARDITEM_H


class StandardServiceRoot;

class StandardItem {
  public:
    explicit StandardItem(StandardServiceRoot *service_root);
    virtual ~StandardItem();

    StandardServiceRoot *serviceRoot() const;
    void setServiceRoot(StandardServiceRoot *service_root);

  protected:
    StandardServiceRoot *m_serviceRoot;
};

#endif // STANDARDITEM_H
