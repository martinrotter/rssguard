// For license of this file, see <project-root-folder>/LICENSE.md.

/* Mimesis -- a library for parsing and creating RFC2822 messages
   Copyright © 2017 Guus Sliepen <guus@lightbts.info>

   Mimesis is free software; you can redistribute it and/or modify it under the
   terms of the GNU Lesser General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
   more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mimesis.hpp"

#include "quoted-printable.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include <QDateTime>
#include <QLocale>
#include <QTextCodec>

using namespace std;

namespace Mimesis {
  static const string base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  static const signed char base64_inverse[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };

  string base64_encode(string_view in) {
    string out;
    size_t outlen = ((in.size() + 2) / 3) * 4;

    out.reserve(outlen);

    size_t i;
    const uint8_t* uin = (const uint8_t*)in.data();

    for (i = 0; i < (in.size() / 3) * 3; i += 3) {
      out.push_back(base64[                         (uin[i + 0] >> 2)]);
      out.push_back(base64[(uin[i + 0] << 4 & 63) | (uin[i + 1] >> 4)]);
      out.push_back(base64[(uin[i + 1] << 2 & 63) | (uin[i + 2] >> 6)]);
      out.push_back(base64[(uin[i + 2] << 0 & 63)                    ]);
    }

    while (i++ < in.size())
      out.push_back('=');

    return out;
  }

  string base64_decode(string_view in) {
    string out;
    size_t estimate = (in.size() / 4) * 3;

    out.reserve(estimate);

    int i = 0;
    uint32_t triplet = 0;

    for(uint8_t c: in) {
      auto d = base64_inverse[c];

      if (d == -1) {
        if (c == '=')
          break;
        else
          continue;
      }

      triplet <<= 6;
      triplet |= d;

      if((i & 3) == 3) {
        out.push_back(static_cast<char>(triplet >> 16));
        out.push_back(static_cast<char>(triplet >> 8));
        out.push_back(static_cast<char>(triplet));
      }

      i++;
    }

    if((i & 3) == 3) {
      out.push_back(static_cast<char>(triplet >> 10));
      out.push_back(static_cast<char>(triplet >> 2));
    }
    else if((i & 3) == 2) {
      out.push_back(static_cast<char>(triplet >> 4));
    }

    return out;
  }

  static std::random_device rnd;

  static string unquote(const string& str) {
    if (str.empty() || str[0] != '"')
      return str;

    string unquoted;
    int quotes_wanted = 2;

    for (auto&& c: str) {
      if (c == '"') {
        if (--quotes_wanted)
          continue;

        break;
      }

      if (c == '\\')
        continue;

      unquoted.push_back(c);
    }

    return unquoted;
  }

  static string quote(const string& str) {
    bool do_quote = false;

    for (auto&& c: str) {
      if (isalnum(c) || strchr("!#$%&'*+-/=?^_`{|}~", c))
        continue;

      do_quote = true;
      break;
    }

    if (!do_quote)
      return str;

    string quoted = "\"";

    for (auto&& c: str) {
      if (c == '\"' || c == '\\')
        quoted.push_back('\\');

      quoted.push_back(c);
    }

    quoted.push_back('"');

    return quoted;
  }

  static bool streqi(const string& a, const string& b) {
    if (a.size() != b.size())
      return false;

    for (size_t i = 0; i < a.size(); i++)
      if (tolower(a[i]) != tolower(b[i]))
        return false;

    return true;
  }

  static bool streqi(const string& a, size_t offset_a, size_t len_a, const string& b) {
    if (min(a.size() - offset_a, len_a) != b.size())
      return false;

    for (size_t i = 0; i < len_a; i++)
      if (tolower(a[i + offset_a]) != tolower(b[i]))
        return false;

    return true;
  }

  static bool streqi(const string& a, size_t offset_a, size_t len_a, const string& b, size_t offset_b, size_t len_b) {
    if (min(a.size() - offset_a, len_a) != min(b.size() - offset_b, len_b))
      return false;

    for (size_t i = 0; i < min(a.size() - offset_a, len_a); i++)
      if (tolower(a[i + offset_a]) != tolower(b[i + offset_b]))
        return false;

    return true;
  }

  static string generate_boundary() {
    unsigned int nonce[24 / sizeof(unsigned int)];

    for (auto& val: nonce)
      val = rnd();

    return base64_encode(string_view(reinterpret_cast<char*>(nonce), sizeof nonce));
  }

  static bool is_boundary(const std::string& line, const std::string& boundary) {
    if (boundary.empty())
      return false;

    if (line.compare(0, 2, "--"))
      return false;

    if (line.compare(2, boundary.size(), boundary))
      return false;

    return true;
  }

  static bool is_final_boundary(const std::string& line, const std::string& boundary) {
    if (line.compare(2 + boundary.size(), 2, "--"))
      return false;

    return is_boundary(line, boundary);
  }

  static bool types_match(const std::string& a, const std::string& b) {
    auto a_slash = a.find('/');
    auto b_slash = b.find('/');

    if (a_slash == string::npos || b_slash == string::npos)
      return streqi(a, 0, a_slash, b, 0, b_slash);
    else
      return streqi(a, b);
  }

  static void set_value(string& str, const string& value) {
    size_t semicolon = str.find(';');

    if (semicolon == string::npos)
      str = value;
    else
      str.replace(0, semicolon, value);
  }

  static string get_value(const string& str) {
    return str.substr(0, str.find(';'));
  }

  static pair<size_t, size_t> get_parameter_value_range(const string& str, const string& parameter) {
    size_t start = 0;
    size_t end = string::npos;

    // Find a semicolon, which marks the start of a parameter.
    while((start = str.find(';', start)) != string::npos) {
      start++;
      while (isspace(str[start]))
        start++;

      if (!streqi(str, start, parameter.size(), parameter)) {
        // It's not the wanted parameter.
        start = str.find('=', start);
        while (isspace(str[start]))
          start++;

        if (str[start] != '=')
          continue;

        while (isspace(str[start]))
          start++;

        // If it's a quoted parameter, skip over the quoted text.
        if (str[start] == '"') {
          start++;
          while (start < str.size() && str[start] != '"') {
            if (str[start] == '\\' && str.size() > start - 1)
              start++;

            start++;
          }
        }

        continue;
      }

      // Skip until we get to the value.
      start += parameter.size();
      while (isspace(str[start]))
        start++;

      if (str[start] != '=')
        continue;

      start++;
      while (isspace(str[start]))
        start++;

      end = start;
      if (str[end] == '"') {
        // It's a quoted parameter.
        end++;
        while (end < str.size() && str[end] != '"') {
          if (str[end] == '\\' && str.size() > end - 1)
            end++;

          end++;
        }
      }
      else {
        while (end < str.size() && str[end] != ';' && !isspace(str[end]))
          end++;
      }

      break;
    }

    return make_pair(start, end);
  }

  static void set_parameter(string& str, const string& parameter, const string& value) {
    auto range = get_parameter_value_range(str, parameter);
    auto start = range.first;
    auto end = range.second;

    if (start == string::npos)
      str += "; " + parameter + "=" + quote(value);
    else
      str.replace(start, end - start, quote(value));
  }

  static string get_parameter(const string& str, const string& parameter) {
    auto range = get_parameter_value_range(str, parameter);
    auto start = range.first;
    auto end = range.second;

    if (start == string::npos)
      return {};

    return unquote(str.substr(start, end - start));
  }

  static const string ending[2] = { "\n", "\r\n" };

  Part::Part() :
    headers(),
    preamble(),
    body(),
    epilogue(),
    parts(),
    boundary(),
    multipart(false),
    crlf(true),
    message(false)
  {}

// Loading and saving a whole MIME message

  string Part::load(istream& in, const string& parent_boundary) {
    string line;
    int ncrlf = 0;
    int nlf = 0;

    while (getline(in, line)) {
      if (is_boundary(line, parent_boundary))
        return line;

      if (line.size() && line.back() == '\r') {
        ncrlf++;
        line.erase(line.size() - 1);
      }
      else {
        nlf++;
      }

      if (line.empty())
        break;

      if (isspace(line[0])) {
        if (headers.empty())
          throw runtime_error("invalid header line");

        headers.back().second.append(line);
        continue;
      }

      size_t colon = string::npos;

      for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == ':') {
          colon = i;
          break;
        }

        if (line[i] < 33 || static_cast<uint8_t>(line[i]) > 127) {
          if (i == 4 && line[i] == ' ' && line.compare(0, 4, "From") == 0 && headers.empty()) {
            colon = i;
            break;
          }

          throw runtime_error("invalid header line " + line + std::to_string(i));
        }
      }

      if (colon == 0 || colon == string::npos)
        throw runtime_error("invalid header line");

      if (line[colon] != ':')
        continue;

      auto start = colon + 1;

      while (start < line.size() && isspace(line[start]))
        start++;

      // Empty header values are allowed for most fields.

      auto field = line.substr(0, colon);
      auto value = line.substr(start);

      headers.emplace_back(field, value);
    }

    crlf = ncrlf > nlf;

    const string content_type = get_header("Content-Type");

    if (types_match(get_value(content_type), "multipart")) {
      boundary = get_parameter(content_type, "boundary");
      if (boundary.empty())
        throw runtime_error("multipart but no boundary specified");

      multipart = true;
    }
    else {
      multipart = false;
    }

    if (!multipart) {
      while (getline(in, line)) {
        if (is_boundary(line, parent_boundary))
          return line;

        line.push_back('\n');
        body.append(line);
      }
    }
    else {
      while (getline(in, line)) {
        if (is_boundary(line, parent_boundary))
          return line;

        if (is_boundary(line, boundary))
          break;

        line.push_back('\n');
        preamble.append(line);
      }

      while (true) {
        parts.emplace_back();
        string last_line = parts.back().load(in, boundary);

        if (!is_boundary(last_line, boundary))
          throw runtime_error("invalid boundary");

        if (is_final_boundary(last_line, boundary))
          break;
      }

      while (getline(in, line)) {
        if (is_boundary(line, parent_boundary))
          return line;

        line.push_back('\n');
        epilogue.append(line);
      }
    }

    if (in.bad())
      throw runtime_error("error reading message");

    return {};
  }

  void Part::save(ostream& out) const {
    bool has_headers = false;

    for (auto& header: headers) {
      if (!header.second.empty()) {
        out << header.first << ": " << header.second << ending[crlf];
        has_headers = true;
      }
    }

    if (message && !has_headers)
      throw runtime_error("no headers specified");

    out << ending[crlf];

    if (parts.empty()) {
      out << body;
    }
    else {
      out << preamble;
      for (auto& part: parts) {
        out << "--" << boundary << ending[crlf];
        part.save(out);
      }

      out << "--" << boundary << "--" << ending[crlf];
      out << epilogue;
    }
  }

  void Part::load(const string& filename) {
    ifstream in(filename);

    if (!in.is_open())
      throw runtime_error("could not open message file");

    load(in);
  }

  void Part::save(const string& filename) const {
    ofstream out(filename);

    if (!out.is_open())
      throw runtime_error("could not open message file");

    save(out);
    out.close();
    if (out.fail())
      throw runtime_error("could not write message file");
  }

  void Part::from_string(const string& data) {
    istringstream in(data);

    load(in);
  }

  string Part::to_string() const {
    ostringstream out;

    save(out);
    return out.str();
  }

  void Part::set_crlf(bool value) {
    crlf = value;
  }

// Low-level access

  string charset_decode(const string& charset, string_view in) {
    QTextCodec* custom_codec = QTextCodec::codecForName(charset.c_str());
    auto unic = custom_codec->toUnicode(string(in).c_str());

    std::string utf8_text = unic.toUtf8().constData();

    return utf8_text;
  }

  string Part::get_body() const {
    string result;
    auto encoding = get_header_value("Content-Transfer-Encoding");

    if (streqi(encoding, "quoted-printable"))
      result = quoted_printable_decode(body);

    if (streqi(encoding, "base64"))
      result = base64_decode(body);
    else
      result = body;

    if (is_mime_type("text")) {
      auto charset = get_header_parameter("Content-Type", "charset");

      if (!charset.empty() && !streqi(charset, "utf-8") && !streqi(charset, "us-ascii") && !streqi(charset, "ascii")) {
        result = charset_decode(charset, result);
      }
    }

    return result;
  }

  string Part::get_preamble() const {
    return preamble;
  }

  string Part::get_epilogue() const {
    return epilogue;
  }

  string Part::get_boundary() const {
    return boundary;
  }

  vector<Part>& Part::get_parts() {
    return parts;
  }

  const vector<Part>& Part::get_parts() const {
    return parts;
  }

  vector<pair<string, string>>& Part::get_headers() {
    return headers;
  }

  const vector<pair<string, string>>& Part::get_headers() const {
    return headers;
  }

  bool Part::is_multipart() const {
    return multipart;
  }

  bool Part::is_multipart(const std::string& subtype) const {
    return multipart && get_header_value("Content-Type") == "multipart/" + subtype;
  }

  bool Part::is_singlepart() const {
    return !multipart;
  }

  bool Part::is_singlepart(const std::string& type) const {
    return !multipart && types_match(get_header_value("Content-Type"), type);
  }

  bool Part::is_attachment() const {
    return get_header_value("Content-Disposition") == "attachment";
  }

  bool Part::is_inline() const {
    return get_header_value("Content-Disposition") == "inline";
  }

  void Part::set_body(const string& value) {
    if (multipart)
      throw runtime_error("Cannot set body of a multipart message");

    body = value;
  }

  void Part::set_preamble(const string& value) {
    if (!multipart)
      throw runtime_error("Cannot set preamble of a non-multipart message");

    preamble = value;
  }

  void Part::set_epilogue(const string& value) {
    if (!multipart)
      throw runtime_error("Cannot set epilogue of a non-multipart message");

    epilogue = value;
  }

  void Part::set_boundary(const std::string& value) {
    boundary = value;
    if (has_mime_type())
      set_header_parameter("Content-Type", "boundary", boundary);
  }

  void Part::set_parts(const vector<Part>& value) {
    if (!multipart)
      throw runtime_error("Cannot set parts of a non-multipart message");

    parts = value;
  }

  void Part::set_headers(const vector<pair<string, string>>& value) {
    headers = value;
  }

  void Part::clear() {
    headers.clear();
    preamble.clear();
    body.clear();
    epilogue.clear();
    parts.clear();
    boundary.clear();
    multipart = false;
  }

  void Part::clear_body() {
    body.clear();
  }

// Header manipulation

  static bool iequals(const string& a, const string& b) {
    if (a.size() != b.size())
      return false;

    for (size_t i = 0; i < a.size(); ++i)
      if (tolower(a[i]) != tolower(b[i]))
        return false;

    return true;
  }

  string Part::get_header(const string& field) const {
    for (const auto& header: headers)
      if (iequals(header.first, field))
        return header.second;

    return {};
  }

  void Part::set_header(const string& field, const string& value) {
    for (auto& header: headers) {
      if (iequals(header.first, field)) {
        header.second = value;
        return;
      }
    }

    append_header(field, value);
  }

  string& Part::operator[](const string& field) {
    for (auto& header: headers)
      if (iequals(header.first, field))
        return header.second;

    append_header(field, {});
    return headers.back().second;
  }

  const string& Part::operator[](const string& field) const {
    for (auto& header: headers)
      if (iequals(header.first, field))
        return header.second;

    static string empty_string;

    return empty_string;
  }

  void Part::append_header(const string& field, const string& value) {
    headers.push_back(make_pair(field, value));
  }

  void Part::prepend_header(const string& field, const string& value) {
    headers.insert(begin(headers), make_pair(field, value));
  }

  void Part::erase_header(const string& field) {
    headers.erase(remove_if(begin(headers), end(headers), [&](pair<string, string>& header) {
      return header.first == field;
    }), end(headers));
  }

  void Part::clear_headers() {
    headers.clear();
  }

  string Part::get_header_value(const string& field) const {
    return get_value(get_header(field));
  }

  string Part::get_header_parameter(const string& field, const string& parameter) const {
    return get_parameter(get_header(field), parameter);
  }

  void Part::set_header_value(const string& field, const string& value) {
    for (auto& header: headers) {
      if (iequals(header.first, field)) {
        set_value(header.second, value);
        return;
      }
    }

    append_header(field, value);
  }

  void Part::set_header_parameter(const string& field, const string& parameter, const string& value) {
    for (auto& header: headers) {
      if (iequals(header.first, field)) {
        set_parameter(header.second, parameter, value);
        return;
      }
    }

    append_header(field, "; " + parameter + "=" + value);
  }

  static string get_date_string(const chrono::system_clock::time_point& date = chrono::system_clock::now()) {

    QLocale loc("C");
    QDateTime dat;

    dat.setSecsSinceEpoch(date.time_since_epoch().count());

    return loc.toString(dat, "ddd, MM MMM yyyy HH:mm:ss t").toStdString();

    /*time_t t = chrono::system_clock::to_time_t(date);
       struct tm tm{};
       localtime_r(&t, &tm);
       char str[128];
       char *oldlocale = setlocale(LC_TIME, "C");
       size_t result = strftime(str, sizeof str, "%a, %d %b %Y %T %z", &tm);
       setlocale(LC_TIME, oldlocale);
       if (result == 0)
       throw runtime_error("Could not convert date to string");
       return str;
     */
  }

  void Part::add_received(const string& text, const chrono::system_clock::time_point& date) {
    prepend_header("Received", text + "; " + get_date_string(date));
  }

  void Part::generate_msgid(const string& domain) {
    auto now = chrono::system_clock::now();
    uint64_t buf[3];

    buf[0] = ((uint64_t)rnd() << 32) | rnd();
    buf[1] = chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()).count();
    buf[2] = ((uint64_t)rnd() << 32) | rnd();
    string msgid = "<" + base64_encode(string_view(reinterpret_cast<char*>(buf), sizeof buf)) + "@" + domain + ">";

    set_header("Message-ID", msgid);
  }

  void Part::set_date(const chrono::system_clock::time_point& date) {
    set_header("Date", get_date_string(date));
  }

// Part manipulation

  Part& Part::append_part(const Part& part) {
    parts.push_back(part);
    return parts.back();
  }

  Part& Part::prepend_part(const Part& part) {
    parts.insert(begin(parts), part);
    return parts.front();
  }

  void Part::clear_parts() {
    parts.clear();
  }

  void Part::make_multipart(const string& subtype, const string& suggested_boundary) {
    if (multipart) {
      if (is_multipart(subtype))
        return;

      Part part;

      part.preamble = move(preamble);
      part.epilogue = move(epilogue);
      part.parts = move(parts);
      part.boundary = move(boundary);
      part.multipart = true;
      part.set_header("Content-Type", get_header("Content-Type"));
      part.set_header("Content-Disposition", get_header("Content-Disposition"));
      erase_header("Content-Disposition");
      part.crlf = crlf;
      parts.emplace_back(move(part));
    }
    else {
      multipart = true;

      if (message)
        set_header("MIME-Version", "1.0");

      if (!body.empty()) {
        auto& part = append_part();

        part.set_header("Content-Type", get_header("Content-Type"));
        part.set_header("Content-Disposition", get_header("Content-Disposition"));
        erase_header("Content-Disposition");
        part.body = move(body);
      }
    }

    if (!suggested_boundary.empty())
      set_boundary(suggested_boundary);

    if (boundary.empty())
      boundary = generate_boundary();

    set_header("Content-Type", "multipart/" + subtype + "; boundary=" + boundary);
  }

  bool Part::flatten() {
    if (!multipart)
      return true;

    if (parts.empty()) {
      multipart = false;
      return true;
    }

    if (parts.size() > 1)
      return false;

    auto& part = parts.front();

    set_header("Content-Type", part.get_header("Content-Type"));
    set_header("Content-Disposition", part.get_header("Content-Disposition"));

    if (part.multipart) {
      parts = move(part.parts);
    }
    else {
      multipart = false;
      set_body(part.get_body());
      parts.clear();
    }

    return true;
  }

// Body and attachments

  string Part::get_mime_type() const {
    return get_header_value("Content-Type");
  }

  void Part::set_mime_type(const std::string& type) {
    return set_header_value("Content-Type", type);
  }

  bool Part::is_mime_type(const std::string& type) const {
    return types_match(get_mime_type(), type);
  }

  bool Part::has_mime_type() const {
    return !get_mime_type().empty();
  }

  const Part* Part::get_first_matching_part(function<bool(const Part&)> predicate) const {
    if (!multipart) {
      if (headers.empty() && body.empty())
        return nullptr;

      if (is_attachment())
        return nullptr;
    }

    if (predicate(*this))
      return this;

    for (auto& part: parts) {
      auto result = part.get_first_matching_part(predicate);

      if (result)
        return result;
    }

    return nullptr;
  }

  Part* Part::get_first_matching_part(function<bool(const Part&)> predicate) {
    auto result = ((const Part*)this)->get_first_matching_part(predicate);

    return const_cast<Part*>(result);
  }

  const Part* Part::get_first_matching_part(const string& type) const {
    return get_first_matching_part([type](const Part& part) {
      auto my_type = part.get_mime_type();
      return types_match(my_type.empty() ? "text/plain" : my_type, type);
    });
  }

  Part* Part::get_first_matching_part(const string& type) {
    auto result = ((const Part*)this)->get_first_matching_part(type);

    return const_cast<Part*>(result);
  }

  string Part::get_first_matching_body(const string& type) const {
    const auto& part = get_first_matching_part(type);

    if (part)
      return part->get_body();
    else
      return {};
  }

  Part& Part::set_alternative(const string& subtype, const string& text) {
    string type = "text/" + subtype;
    Part* part = nullptr;

    // Try to put it in the body first.
    if (!multipart) {
      if (body.empty() || is_mime_type(type)) {
        part = this;
      }
      else if (is_mime_type("text") && !is_attachment()) {
        make_multipart("alternative");
        part = &append_part();
      }
      else {
        make_multipart("mixed");
        part = &prepend_part();
      }
    }
    else {
      // If there is already a text/plain part, use that one.
      part = get_first_matching_part(type);
      if (part) {
        part->set_mime_type(type);
        part->set_body(text);
        return *part;
      }

      // If there is already a multipart/alternative with text, use that one.
      part = get_first_matching_part([](const Part& part) {
        return part.is_multipart("alternative")
        && !part.parts.empty()
        && part.get_first_matching_part("text");
      });
      if (part)
        part = &part->append_part();

      // If there is already inline text, make it multipart/alternative.

      if (!part && (part = get_first_matching_part("text"))) {
        part->make_multipart("alternative");
        part = &part->append_part();
      }

      // Otherwise, assume we're multipart/mixed.
      if (!part)
        part = &prepend_part();
    }

    part->set_header("Content-Type", type);
    part->set_body(text);

    return *part;
  }

  void Part::set_plain(const string& text) {
    set_alternative("plain", text);
  }

  void Part::set_html(const string& html) {
    set_alternative("html", html);
  }

  string Part::get_plain() const {
    return get_first_matching_body("text/plain");
  }

  string Part::get_html() const {
    return get_first_matching_body("text/html");
  }

  string Part::get_text() const {
    return get_first_matching_body("text");
  }

  Part& Part::attach(const Part& attachment) {
    if (!multipart && body.empty()) {
      if (attachment.message) {
        set_header("Content-Type", "message/rfc822");
        body = attachment.to_string();
      }
      else {
        set_header("Content-Type", attachment.get_header("Content-Type"));
        body = attachment.body;
      }

      set_header("Content-Disposition", "attachment");
      return *this;
    }

    make_multipart("mixed");
    auto& part = append_part();

    if (attachment.message) {
      part.set_header("Content-Type", "message/rfc822");
      part.body = attachment.to_string();
    }
    else {
      part.set_header("Content-Type", attachment.get_header("Content-Type"));
      part.body = attachment.body;
    }

    part.set_header("Content-Disposition", "attachment");
    return part;
  }

  Part& Part::attach(const string& data, const string& type, const string& filename) {
    if (!multipart && body.empty()) {
      set_header("Content-Type", type.empty() ? "text/plain" : type);
      set_header("Content-Disposition", "attachment");
      if (!filename.empty())
        set_header_parameter("Content-Disposition", "filename", filename);

      body = data;
      return *this;
    }

    make_multipart("mixed");
    auto& part = append_part();

    part.set_header("Content-Type", type.empty() ? "text/plain" : type);
    part.set_header("Content-Disposition", "attachment");
    if (!filename.empty())
      part.set_header_parameter("Content-Disposition", "filename", filename);

    part.set_body(data);
    return part;
  }

  Part& Part::attach(istream& in, const string& type, const string& filename) {
    auto& part = attach("", type, filename);
    char buffer[4096];

    while (in.read(buffer, sizeof(buffer)))
      part.body.append(buffer, sizeof(buffer));

    part.body.append(buffer, in.gcount());
    return part;
  }

  vector<const Part*> Part::get_attachments() const {
    vector<const Part*> attachments;

    if (!multipart && get_header_value("Content-Disposition") == "attachment") {
      attachments.push_back(this);
      return attachments;
    }

    for (auto& part: parts) {
      auto sub = part.get_attachments();

      attachments.insert(end(attachments), begin(sub), end(sub));
    }

    return attachments;
  }

  void Part::simplify() {
    if (!multipart)
      return;

    for (auto& part: parts)
      part.simplify();

    parts.erase(remove_if(begin(parts), end(parts), [&](Part& part) {
      return part.headers.empty() && part.body.empty();
    }), end(parts));

    if (parts.empty()) {
      if (message) {
        erase_header("Content-Type");
        erase_header("Content-Disposition");
        multipart = false;
      }
      else {
        clear();
      }
    }
    else if (parts.size() == 1) {
      flatten();
    }
  }

  void Part::clear_attachments() {
    if (!multipart) {
      if (get_header_value("Content-Disposition") == "attachment") {
        if (message) {
          erase_header("Content-Type");
          erase_header("Content-Disposition");
          body.clear();
        }
        else {
          clear();
        }
      }
    }
    else {
      for (auto& part: parts)
        part.clear_attachments();

      simplify();
    }
  }

  void Part::clear_alternative(const string& type) {
    bool cleared = false;
    Part* part;

    while((part = get_first_matching_part(type))) {
      part->clear();
      cleared = true;
    }

    if (cleared)
      simplify();
  }

  void Part::clear_text() {
    clear_alternative("text");
  }

  void Part::clear_plain() {
    clear_alternative("text/plain");
  }

  void Part::clear_html() {
    clear_alternative("text/html");
  }

  bool Part::has_text() const {
    return get_first_matching_part("text");
  }

  bool Part::has_plain() const {
    return get_first_matching_part("text/plain");
  }

  bool Part::has_html() const {
    return get_first_matching_part("text/html");
  }

  bool Part::has_attachments() const {
    if (is_attachment())
      return true;

    for (auto& part: parts)
      if (part.has_attachments())
        return true;

    return false;
  }

// RFC2822 messages

  Message::Message() {
    message = true;
  }

// Comparison

  bool operator==(const Part& lhs, const Part& rhs) {
    return lhs.crlf == rhs.crlf && lhs.multipart == rhs.multipart &&
           lhs.preamble == rhs.preamble && lhs.body == rhs.body &&
           lhs.epilogue == rhs.epilogue && lhs.boundary == rhs.boundary &&
           lhs.headers == rhs.headers && lhs.parts == rhs.parts;
  }

  bool operator!=(const Part& lhs, const Part& rhs) {
    return !(lhs == rhs);
  }

}
