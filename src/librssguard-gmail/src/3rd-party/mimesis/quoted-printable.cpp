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

#include "quoted-printable.hpp"

#include <cstdint>

using namespace std;

string quoted_printable_decode(string_view in) {
  string out;

  out.reserve(in.size());

  int decode = 0;
  uint8_t val = 0;

  for (auto&& c : in) {
    if (decode) {
      if (c >= '0' && c <= '9') {
        val <<= 4;
        val |= c - '0';
        decode--;
      }
      else if (c >= 'A' && c <= 'F') {
        val <<= 4;
        val |= 10 + (c - 'A');
        decode--;
      }
      else {
        decode = 0;
        continue;
      }

      if (decode == 0) {
        out.push_back(static_cast<char>(val));
      }
    }
    else {
      if (c == '=') {
        decode = 2;
      }
      else {
        out.push_back(c);
      }
    }
  }

  return out;
}
