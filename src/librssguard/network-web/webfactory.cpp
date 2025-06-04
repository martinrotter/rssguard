// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/webfactory.h"

#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QDesktopServices>
#include <QElapsedTimer>
#include <QProcess>
#include <QUrl>

WebFactory::WebFactory(QObject* parent) : QObject(parent), m_customUserAgent(QString()) {}

WebFactory::~WebFactory() {}

bool WebFactory::sendMessageViaEmail(const Message& message) {
  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailEnabled)).toBool()) {
    const QString browser =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailExecutable)).toString();
    const QString arguments =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailArguments)).toString();
    const QStringList tokenized_arguments =
      TextFactory::tokenizeProcessArguments(arguments.arg(message.m_title, stripTags(message.m_contents)));

    return IOFactory::startProcessDetached(browser, tokenized_arguments);
  }
  else {
    // Send it via mailto protocol.
    // NOTE: http://en.wikipedia.org/wiki/Mailto
    return QDesktopServices::openUrl(QSL("mailto:?subject=%1&body=%2")
                                       .arg(QString(QUrl::toPercentEncoding(message.m_title)),
                                            QString(QUrl::toPercentEncoding(stripTags(message.m_contents)))));
  }
}

bool WebFactory::openUrlInExternalBrowser(const QUrl& url) const {
  QString my_url = url.toString(QUrl::ComponentFormattingOption::FullyEncoded);

  qDebugNN << LOGSEC_NETWORK << "We are trying to open URL" << QUOTE_W_SPACE_DOT(my_url);

  bool result = false;

  if (qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserEnabled)).toBool()) {
    const QString browser =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserExecutable)).toString();
    const QString arguments =
      qApp->settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserArguments)).toString();
    const auto nice_args = arguments.arg(my_url);

    qDebugNN << LOGSEC_NETWORK << "Arguments for external browser:" << QUOTE_W_SPACE_DOT(nice_args);

    result = IOFactory::startProcessDetached(browser, TextFactory::tokenizeProcessArguments(nice_args));

    if (!result) {
      qDebugNN << LOGSEC_NETWORK << "External web browser call failed.";
    }
  }
  else {
    result = QDesktopServices::openUrl(my_url);
  }

  if (!result) {
    // We display GUI information that browser was not probably opened.
    MsgBox::show(qApp->mainFormWidget(),
                 QMessageBox::Icon::Critical,
                 tr("Navigate to website manually"),
                 tr("%1 was unable to launch your web browser with the given URL, you need to open the "
                    "below website URL in your web browser manually.")
                   .arg(QSL(APP_NAME)),
                 {},
                 my_url,
                 QMessageBox::StandardButton::Ok);
  }

  return result;
}

QString WebFactory::stripTags(QString text) {
  static QRegularExpression reg_tags(QSL("<[^>]*>"));

  return text.remove(reg_tags);
}

QString WebFactory::unescapeHtml(const QString& html) {
  if (html.isEmpty()) {
    return html;
  }

  static QMap<QString, char16_t> entities = generateUnescapes();

  QString output;
  output.reserve(html.size());

  // Traverse input HTML string and replace named/number entities.
  for (int pos = 0; pos < html.size();) {
    const QChar first = html.at(pos);

    if (first == QChar('&')) {
      // We need to find ending ';'.
      int pos_end = -1;

      // We're finding end of entity, but also we limit searching window to 10 characters.
      for (int pos_find = pos; pos_find <= pos + 10 && pos_find < html.size(); pos_find++) {
        if (html.at(pos_find) == QChar(';')) {
          // We found end of the entity.
          pos_end = pos_find;
          break;
        }
      }

      if (pos_end + 1 > pos) {
        // OK, we have entity.
        if (html.at(pos + 1) == QChar('#')) {
          // We have numbered entity.
          uint number;
          QString number_str;

          if (html.at(pos + 2) == QChar('x')) {
            // base-16 number.
            number_str = html.mid(pos + 3, pos_end - pos - 3);
            number = number_str.toUInt(nullptr, 16);
          }
          else {
            // base-10 number.
            number_str = html.mid(pos + 2, pos_end - pos - 2);
            number = number_str.toUInt();
          }

          if (number > 0U) {
            output.append(QString::fromUcs4((const char32_t*)&number, 1));
          }
          else {
            // Failed to convert to number, leave intact.
            output.append(html.mid(pos, pos_end - pos + 1));
          }

          pos = pos_end + 1;
          continue;
        }
        else {
          // We have named entity.
          auto entity_name = html.mid(pos + 1, pos_end - pos - 1);

          if (entities.contains(entity_name)) {
            // Entity found, proceed.
            output.append(entities.value(entity_name));
          }
          else {
            // Entity NOT found, leave intact.
            output.append('&');
            output.append(entity_name);
            output.append(';');
          }

          pos = pos_end + 1;
          continue;
        }
      }
    }

    // No entity, normally append and continue.
    output.append(first);
    pos++;
  }

  /*
     qDebugNN << LOGSEC_CORE
           << "Unescaped string" << QUOTE_W_SPACE(html)
           << "to" << QUOTE_W_SPACE_DOT(output);
   */

  return output;
}

QString WebFactory::processFeedUriScheme(const QString& url) {
  if (url.startsWith(QSL(URI_SCHEME_FEED))) {
    return QSL(URI_SCHEME_HTTPS) + url.mid(QSL(URI_SCHEME_FEED).size());
  }
  else if (url.startsWith(QSL(URI_SCHEME_FEED_SHORT))) {
    return url.mid(QSL(URI_SCHEME_FEED_SHORT).size());
  }
  else {
    return url;
  }
}

void WebFactory::updateProxy() {
  const QNetworkProxy::ProxyType selected_proxy_type =
    static_cast<QNetworkProxy::ProxyType>(qApp->settings()->value(GROUP(Proxy), SETTING(Proxy::Type)).toInt());

  if (selected_proxy_type == QNetworkProxy::NoProxy) {
    qDebugNN << LOGSEC_NETWORK << "Disabling application-wide proxy completely.";

    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::ProxyType::NoProxy);
  }
  else if (selected_proxy_type == QNetworkProxy::ProxyType::DefaultProxy) {
    qDebugNN << LOGSEC_NETWORK << "Using application-wide proxy to be system's default proxy.";
    QNetworkProxyFactory::setUseSystemConfiguration(true);
  }
  else {
    const Settings* settings = qApp->settings();
    QNetworkProxy new_proxy;

    // Custom proxy is selected, set it up.
    new_proxy.setType(selected_proxy_type);
    new_proxy.setHostName(settings->value(GROUP(Proxy), SETTING(Proxy::Host)).toString());
    new_proxy.setPort(quint16(settings->value(GROUP(Proxy), SETTING(Proxy::Port)).toInt()));
    new_proxy.setUser(settings->value(GROUP(Proxy), SETTING(Proxy::Username)).toString());
    new_proxy.setPassword(settings->password(GROUP(Proxy), SETTING(Proxy::Password)).toString());

    qWarningNN << LOGSEC_NETWORK
               << "Activating application-wide custom proxy, address:" << QUOTE_W_SPACE_COMMA(new_proxy.hostName())
               << " type:" << QUOTE_W_SPACE_DOT(new_proxy.type());

    QNetworkProxy::setApplicationProxy(new_proxy);
  }
}

QMap<QString, char16_t> WebFactory::generateUnescapes() {
  QMap<QString, char16_t> res;
  res[QSL("AElig")] = 0x00c6;
  res[QSL("AMP")] = 38;
  res[QSL("Aacute")] = 0x00c1;
  res[QSL("Acirc")] = 0x00c2;
  res[QSL("Agrave")] = 0x00c0;
  res[QSL("Alpha")] = 0x0391;
  res[QSL("Aring")] = 0x00c5;
  res[QSL("Atilde")] = 0x00c3;
  res[QSL("Auml")] = 0x00c4;
  res[QSL("Beta")] = 0x0392;
  res[QSL("Ccedil")] = 0x00c7;
  res[QSL("Chi")] = 0x03a7;
  res[QSL("Dagger")] = 0x2021;
  res[QSL("Delta")] = 0x0394;
  res[QSL("ETH")] = 0x00d0;
  res[QSL("Eacute")] = 0x00c9;
  res[QSL("Ecirc")] = 0x00ca;
  res[QSL("Egrave")] = 0x00c8;
  res[QSL("Epsilon")] = 0x0395;
  res[QSL("Eta")] = 0x0397;
  res[QSL("Euml")] = 0x00cb;
  res[QSL("GT")] = 62;
  res[QSL("Gamma")] = 0x0393;
  res[QSL("Iacute")] = 0x00cd;
  res[QSL("Icirc")] = 0x00ce;
  res[QSL("Igrave")] = 0x00cc;
  res[QSL("Iota")] = 0x0399;
  res[QSL("Iuml")] = 0x00cf;
  res[QSL("Kappa")] = 0x039a;
  res[QSL("LT")] = 60;
  res[QSL("Lambda")] = 0x039b;
  res[QSL("Mu")] = 0x039c;
  res[QSL("Ntilde")] = 0x00d1;
  res[QSL("Nu")] = 0x039d;
  res[QSL("OElig")] = 0x0152;
  res[QSL("Oacute")] = 0x00d3;
  res[QSL("Ocirc")] = 0x00d4;
  res[QSL("Ograve")] = 0x00d2;
  res[QSL("Omega")] = 0x03a9;
  res[QSL("Omicron")] = 0x039f;
  res[QSL("Oslash")] = 0x00d8;
  res[QSL("Otilde")] = 0x00d5;
  res[QSL("Ouml")] = 0x00d6;
  res[QSL("Phi")] = 0x03a6;
  res[QSL("Pi")] = 0x03a0;
  res[QSL("Prime")] = 0x2033;
  res[QSL("Psi")] = 0x03a8;
  res[QSL("QUOT")] = 34;
  res[QSL("Rho")] = 0x03a1;
  res[QSL("Scaron")] = 0x0160;
  res[QSL("Sigma")] = 0x03a3;
  res[QSL("THORN")] = 0x00de;
  res[QSL("Tau")] = 0x03a4;
  res[QSL("Theta")] = 0x0398;
  res[QSL("Uacute")] = 0x00da;
  res[QSL("Ucirc")] = 0x00db;
  res[QSL("Ugrave")] = 0x00d9;
  res[QSL("Upsilon")] = 0x03a5;
  res[QSL("Uuml")] = 0x00dc;
  res[QSL("Xi")] = 0x039e;
  res[QSL("Yacute")] = 0x00dd;
  res[QSL("Yuml")] = 0x0178;
  res[QSL("Zeta")] = 0x0396;
  res[QSL("aacute")] = 0x00e1;
  res[QSL("acirc")] = 0x00e2;
  res[QSL("acute")] = 0x00b4;
  res[QSL("aelig")] = 0x00e6;
  res[QSL("agrave")] = 0x00e0;
  res[QSL("alefsym")] = 0x2135;
  res[QSL("alpha")] = 0x03b1;
  res[QSL("amp")] = 38;
  res[QSL("and")] = 0x22a5;
  res[QSL("ang")] = 0x2220;
  res[QSL("apos")] = 0x0027;
  res[QSL("aring")] = 0x00e5;
  res[QSL("asymp")] = 0x2248;
  res[QSL("atilde")] = 0x00e3;
  res[QSL("auml")] = 0x00e4;
  res[QSL("bdquo")] = 0x201e;
  res[QSL("beta")] = 0x03b2;
  res[QSL("brvbar")] = 0x00a6;
  res[QSL("bull")] = 0x2022;
  res[QSL("cap")] = 0x2229;
  res[QSL("ccedil")] = 0x00e7;
  res[QSL("cedil")] = 0x00b8;
  res[QSL("cent")] = 0x00a2;
  res[QSL("chi")] = 0x03c7;
  res[QSL("circ")] = 0x02c6;
  res[QSL("clubs")] = 0x2663;
  res[QSL("cong")] = 0x2245;
  res[QSL("copy")] = 0x00a9;
  res[QSL("crarr")] = 0x21b5;
  res[QSL("cup")] = 0x222a;
  res[QSL("curren")] = 0x00a4;
  res[QSL("dArr")] = 0x21d3;
  res[QSL("dagger")] = 0x2020;
  res[QSL("darr")] = 0x2193;
  res[QSL("deg")] = 0x00b0;
  res[QSL("delta")] = 0x03b4;
  res[QSL("diams")] = 0x2666;
  res[QSL("divide")] = 0x00f7;
  res[QSL("eacute")] = 0x00e9;
  res[QSL("ecirc")] = 0x00ea;
  res[QSL("egrave")] = 0x00e8;
  res[QSL("empty")] = 0x2205;
  res[QSL("emsp")] = 0x2003;
  res[QSL("ensp")] = 0x2002;
  res[QSL("epsilon")] = 0x03b5;
  res[QSL("equiv")] = 0x2261;
  res[QSL("eta")] = 0x03b7;
  res[QSL("eth")] = 0x00f0;
  res[QSL("euml")] = 0x00eb;
  res[QSL("euro")] = 0x20ac;
  res[QSL("exist")] = 0x2203;
  res[QSL("fnof")] = 0x0192;
  res[QSL("forall")] = 0x2200;
  res[QSL("frac12")] = 0x00bd;
  res[QSL("frac14")] = 0x00bc;
  res[QSL("frac34")] = 0x00be;
  res[QSL("frasl")] = 0x2044;
  res[QSL("gamma")] = 0x03b3;
  res[QSL("ge")] = 0x2265;
  res[QSL("gt")] = 62;
  res[QSL("hArr")] = 0x21d4;
  res[QSL("harr")] = 0x2194;
  res[QSL("hearts")] = 0x2665;
  res[QSL("hellip")] = 0x2026;
  res[QSL("iacute")] = 0x00ed;
  res[QSL("icirc")] = 0x00ee;
  res[QSL("iexcl")] = 0x00a1;
  res[QSL("igrave")] = 0x00ec;
  res[QSL("image")] = 0x2111;
  res[QSL("infin")] = 0x221e;
  res[QSL("int")] = 0x222b;
  res[QSL("iota")] = 0x03b9;
  res[QSL("iquest")] = 0x00bf;
  res[QSL("isin")] = 0x2208;
  res[QSL("iuml")] = 0x00ef;
  res[QSL("kappa")] = 0x03ba;
  res[QSL("lArr")] = 0x21d0;
  res[QSL("lambda")] = 0x03bb;
  res[QSL("lang")] = 0x2329;
  res[QSL("laquo")] = 0x00ab;
  res[QSL("larr")] = 0x2190;
  res[QSL("lceil")] = 0x2308;
  res[QSL("ldquo")] = 0x201c;
  res[QSL("le")] = 0x2264;
  res[QSL("lfloor")] = 0x230a;
  res[QSL("lowast")] = 0x2217;
  res[QSL("loz")] = 0x25ca;
  res[QSL("lrm")] = 0x200e;
  res[QSL("lsaquo")] = 0x2039;
  res[QSL("lsquo")] = 0x2018;
  res[QSL("lt")] = 60;
  res[QSL("macr")] = 0x00af;
  res[QSL("mdash")] = 0x2014;
  res[QSL("micro")] = 0x00b5;
  res[QSL("middot")] = 0x00b7;
  res[QSL("minus")] = 0x2212;
  res[QSL("mu")] = 0x03bc;
  res[QSL("nabla")] = 0x2207;
  res[QSL("nbsp")] = 0x00a0;
  res[QSL("ndash")] = 0x2013;
  res[QSL("ne")] = 0x2260;
  res[QSL("ni")] = 0x220b;
  res[QSL("not")] = 0x00ac;
  res[QSL("notin")] = 0x2209;
  res[QSL("nsub")] = 0x2284;
  res[QSL("ntilde")] = 0x00f1;
  res[QSL("nu")] = 0x03bd;
  res[QSL("oacute")] = 0x00f3;
  res[QSL("ocirc")] = 0x00f4;
  res[QSL("oelig")] = 0x0153;
  res[QSL("ograve")] = 0x00f2;
  res[QSL("oline")] = 0x203e;
  res[QSL("omega")] = 0x03c9;
  res[QSL("omicron")] = 0x03bf;
  res[QSL("oplus")] = 0x2295;
  res[QSL("or")] = 0x22a6;
  res[QSL("ordf")] = 0x00aa;
  res[QSL("ordm")] = 0x00ba;
  res[QSL("oslash")] = 0x00f8;
  res[QSL("otilde")] = 0x00f5;
  res[QSL("otimes")] = 0x2297;
  res[QSL("ouml")] = 0x00f6;
  res[QSL("para")] = 0x00b6;
  res[QSL("part")] = 0x2202;
  res[QSL("percnt")] = 0x0025;
  res[QSL("permil")] = 0x2030;
  res[QSL("perp")] = 0x22a5;
  res[QSL("phi")] = 0x03c6;
  res[QSL("pi")] = 0x03c0;
  res[QSL("piv")] = 0x03d6;
  res[QSL("plusmn")] = 0x00b1;
  res[QSL("pound")] = 0x00a3;
  res[QSL("prime")] = 0x2032;
  res[QSL("prod")] = 0x220f;
  res[QSL("prop")] = 0x221d;
  res[QSL("psi")] = 0x03c8;
  res[QSL("quot")] = 34;
  res[QSL("rArr")] = 0x21d2;
  res[QSL("radic")] = 0x221a;
  res[QSL("rang")] = 0x232a;
  res[QSL("raquo")] = 0x00bb;
  res[QSL("rarr")] = 0x2192;
  res[QSL("rceil")] = 0x2309;
  res[QSL("rdquo")] = 0x201d;
  res[QSL("real")] = 0x211c;
  res[QSL("reg")] = 0x00ae;
  res[QSL("rfloor")] = 0x230b;
  res[QSL("rho")] = 0x03c1;
  res[QSL("rlm")] = 0x200f;
  res[QSL("rsaquo")] = 0x203a;
  res[QSL("rsquo")] = 0x2019;
  res[QSL("sbquo")] = 0x201a;
  res[QSL("scaron")] = 0x0161;
  res[QSL("sdot")] = 0x22c5;
  res[QSL("sect")] = 0x00a7;
  res[QSL("shy")] = 0x00ad;
  res[QSL("sigma")] = 0x03c3;
  res[QSL("sigmaf")] = 0x03c2;
  res[QSL("sim")] = 0x223c;
  res[QSL("spades")] = 0x2660;
  res[QSL("sub")] = 0x2282;
  res[QSL("sube")] = 0x2286;
  res[QSL("sum")] = 0x2211;
  res[QSL("sup")] = 0x2283;
  res[QSL("sup1")] = 0x00b9;
  res[QSL("sup2")] = 0x00b2;
  res[QSL("sup3")] = 0x00b3;
  res[QSL("supe")] = 0x2287;
  res[QSL("szlig")] = 0x00df;
  res[QSL("tau")] = 0x03c4;
  res[QSL("there4")] = 0x2234;
  res[QSL("theta")] = 0x03b8;
  res[QSL("thetasym")] = 0x03d1;
  res[QSL("thinsp")] = 0x2009;
  res[QSL("thorn")] = 0x00fe;
  res[QSL("tilde")] = 0x02dc;
  res[QSL("times")] = 0x00d7;
  res[QSL("trade")] = 0x2122;
  res[QSL("uArr")] = 0x21d1;
  res[QSL("uacute")] = 0x00fa;
  res[QSL("uarr")] = 0x2191;
  res[QSL("ucirc")] = 0x00fb;
  res[QSL("ugrave")] = 0x00f9;
  res[QSL("uml")] = 0x00a8;
  res[QSL("upsih")] = 0x03d2;
  res[QSL("upsilon")] = 0x03c5;
  res[QSL("uuml")] = 0x00fc;
  res[QSL("weierp")] = 0x2118;
  res[QSL("xi")] = 0x03be;
  res[QSL("yacute")] = 0x00fd;
  res[QSL("yen")] = 0x00a5;
  res[QSL("yuml")] = 0x00ff;
  res[QSL("zeta")] = 0x03b6;
  res[QSL("zwj")] = 0x200d;
  res[QSL("zwnj")] = 0x200c;

  return res;
}

QString WebFactory::customUserAgent() const {
  return m_customUserAgent;
}

void WebFactory::setCustomUserAgent(const QString& user_agent) {
  m_customUserAgent = user_agent;
}
