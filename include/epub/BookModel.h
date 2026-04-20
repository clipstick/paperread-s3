#pragma once

#include <string>
#include <vector>

/**
 * Data structures that describe a parsed EPUB book.
 *
 * These are plain data objects with no behaviour - the OpfParser
 * populates them and consumers read them.
 */

struct BookMetadata {
    std::string title;
    std::string author;
    std::string language;
    std::string cover_href;   // path inside the ZIP to the cover image (may be empty)
};

struct SpineItem {
    std::string id;           // manifest id
    std::string href;         // full path inside the ZIP
};

struct TocItem {
    std::string title;
    std::string href;         // full path inside the ZIP (without fragment)
    std::string anchor;       // fragment identifier (may be empty)
    std::vector<TocItem> children;
};

struct BookModel {
    BookMetadata metadata;
    std::vector<SpineItem> spine;
    std::vector<TocItem> toc;
    std::string base_path;    // directory prefix for resolving relative hrefs
};
