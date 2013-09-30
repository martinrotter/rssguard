#ifndef TEXTFACTORY_H
#define TEXTFACTORY_H


class TextFactory {
  private:
    explicit TextFactory();

  public:
    virtual ~TextFactory();

    static QString shorten(const QString &input);
};

#endif // TEXTFACTORY_H
