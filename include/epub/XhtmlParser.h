#pragma once

#include "BlockModel.h"
#include <cstddef>

namespace epub {

class XhtmlParser {
public:
    /// Parse XHTML content into a list of content blocks.
    /// Never throws; returns an empty list on catastrophic parse failure.
    static BlockList parse(const char* xhtml_data, size_t length);
};

} // namespace epub
