#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace epub {

struct TextStyle {
    bool bold       = false;
    bool italic     = false;
    int8_t font_size_delta = 0;   // relative to base size (e.g. +2 for headings)
    bool is_link    = false;

    bool operator==(const TextStyle& o) const {
        return bold == o.bold
            && italic == o.italic
            && font_size_delta == o.font_size_delta
            && is_link == o.is_link;
    }
    bool operator!=(const TextStyle& o) const { return !(*this == o); }
};

struct TextRun {
    std::string text;
    TextStyle   style;
};

enum class BlockType : uint8_t {
    PARAGRAPH,
    HEADING_1,
    HEADING_2,
    HEADING_3,
    HEADING_4,
    HEADING_5,
    HEADING_6,
    IMAGE,
    HORIZONTAL_RULE,
    LIST_ITEM,
    BLOCKQUOTE,
};

struct Block {
    BlockType              type = BlockType::PARAGRAPH;
    std::vector<TextRun>   runs;
    std::string            image_src;
    uint8_t                heading_level = 0;  // 1-6 for headings, 0 otherwise

    bool is_empty() const {
        if (type == BlockType::IMAGE || type == BlockType::HORIZONTAL_RULE) {
            return false;
        }
        for (const auto& r : runs) {
            for (char c : r.text) {
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    return false;
                }
            }
        }
        return true;
    }
};

using BlockList = std::vector<Block>;

} // namespace epub
