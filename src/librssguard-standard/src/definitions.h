#ifndef STANDARD_DEFINITIONS_H
#define STANDARD_DEFINITIONS_H

#define DEFAULT_FEED_ENCODING       "UTF-8"
#define DEFAULT_FEED_TYPE           "RSS"
#define FEED_INITIAL_OPML_PATTERN   "feeds-%1.opml"
#define DEFAULT_ENCLOSURE_MIME_TYPE "image/jpg"
#define LOGSEC_STANDARD             "standard: "

#define ADVANCED_FEED_ADD_DIALOG_CODE 64

#define RSS_REGEX_MATCHER      "<link[^>]+type=\"application\\/(?:rss\\+xml)\"[^>]*>"
#define RSS_HREF_REGEX_MATCHER "href=\"([^\"]+)\""

#define JSON_REGEX_MATCHER      "<link[^>]+type=\"application\\/(?:feed\\+json|json)\"[^>]*>"
#define JSON_HREF_REGEX_MATCHER "href=\"([^\"]+)\""

#define ATOM_REGEX_MATCHER      "<link[^>]+type=\"application\\/(?:atom\\+xml|rss\\+xml)\"[^>]*>"
#define ATOM_HREF_REGEX_MATCHER "href=\"([^\"]+)\""

#define GITHUB_URL_REGEX "github\\.com\\/([^\\s]+)\\/([^\\s]+)"

#endif // STANDARD_DEFINITIONS_H
