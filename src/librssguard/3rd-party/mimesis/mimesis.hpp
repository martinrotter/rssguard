// For license of this file, see <project-root-folder>/LICENSE.md.

#pragma once

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

#include <chrono>
#include <functional>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

namespace Mimesis {

  std::string base64_encode(std::string_view in);
  std::string base64_decode(std::string_view in);

  class Part {
    std::vector<std::pair<std::string, std::string>> headers;
    std::string preamble;
    std::string body;
    std::string epilogue;
    std::vector<Part> parts;
    std::string boundary;
    bool multipart;
    bool crlf;

    protected:
      bool message;

    public:
      Part();
      friend bool operator==(const Part& lhs, const Part& rhs);
      friend bool operator!=(const Part& lhs, const Part& rhs);

      // Loading and saving a whole MIME message
      std::string load(std::istream& in, const std::string& parent_boundary = {});
      void load(const std::string& filename);
      void save(std::ostream& out) const;
      void save(const std::string& filename) const;
      void from_string(const std::string& data);
      std::string to_string() const;

      // Low-level access
      std::string get_body() const;
      std::string get_preamble() const;
      std::string get_epilogue() const;
      std::string get_boundary() const;
      std::vector<Part>& get_parts();
      const std::vector<Part>& get_parts() const;
      std::vector<std::pair<std::string, std::string>>& get_headers();
      const std::vector<std::pair<std::string, std::string>>& get_headers() const;

      bool is_multipart() const;
      bool is_multipart(const std::string& subtype) const;
      bool is_singlepart() const;
      bool is_singlepart(const std::string& type) const;

      void set_body(const std::string& body);
      void set_preamble(const std::string& preamble);
      void set_epilogue(const std::string& epilogue);
      void set_boundary(const std::string& boundary);
      void set_parts(const std::vector<Part>& parts);
      void set_headers(const std::vector<std::pair<std::string, std::string>>& headers);

      void clear();
      void clear_body();

      // Header manipulation
      std::string get_header(const std::string& field) const;
      void set_header(const std::string& field, const std::string& value);
      std::string& operator[](const std::string& field);
      const std::string& operator[](const std::string& field) const;

      void append_header(const std::string& field, const std::string& value);
      void prepend_header(const std::string& field, const std::string& value);
      void erase_header(const std::string& field);
      void clear_headers();

      // Specialized header functions
      std::string get_multipart_type() const;
      std::string get_header_value(const std::string& field) const;
      std::string get_header_parameter(const std::string& field, const std::string& parameter) const;

      void set_header_value(const std::string& field, const std::string& value);
      void set_header_parameter(const std::string& field, const std::string& paramter, const std::string& value);

      void add_received(const std::string& domain, const std::chrono::system_clock::time_point& date = std::chrono::system_clock::now());
      void generate_msgid(const std::string& domain);
      void set_date(const std::chrono::system_clock::time_point& date = std::chrono::system_clock::now());

      // Part manipulation
      Part& append_part(const Part& part = {});
      Part& prepend_part(const Part& part = {});

      void clear_parts();
      void make_multipart(const std::string& type, const std::string& boundary = {});
      bool flatten();

      std::string get_mime_type() const;
      void set_mime_type(const std::string& type);
      bool is_mime_type(const std::string& type) const;
      bool has_mime_type() const;

      // Body and attachments
      Part& set_alternative(const std::string& subtype, const std::string& text);

      void set_plain(const std::string& text);
      void set_html(const std::string& text);

      const Part* get_first_matching_part(std::function<bool(const Part&)> predicate) const;

      Part* get_first_matching_part(std::function<bool(const Part&)> predicate);
      const Part* get_first_matching_part(const std::string& type) const;

      Part* get_first_matching_part(const std::string& type);
      std::string get_first_matching_body(const std::string& type) const;
      std::string get_text() const;
      std::string get_plain() const;
      std::string get_html() const;

      Part& attach(const Part& attachment);
      Part& attach(const std::string& data, const std::string& mime_type, const std::string& filename = {});
      Part& attach(std::istream& in, const std::string& mime_type, const std::string& filename = {});

      std::vector<const Part*> get_attachments() const;

      void clear_alternative(const std::string& subtype);
      void clear_text();
      void clear_plain();
      void clear_html();
      void clear_attachments();

      void simplify();

      bool has_text() const;
      bool has_plain() const;
      bool has_html() const;
      bool has_attachments() const;
      bool is_attachment() const;
      bool is_inline() const;

      // Format manipulation
      void set_crlf(bool value = true);
  };

  class Message : public Part {
    public:
      Message();
  };

  bool operator==(const Part& lhs, const Part& rhs);
  bool operator!=(const Part& lhs, const Part& rhs);

}

inline std::ostream& operator<<(std::ostream& out, const Mimesis::Part& part) {
  part.save(out);
  return out;
}

inline std::istream& operator>>(std::istream& in, Mimesis::Part& part) {
  part.load(in);
  return in;
}
