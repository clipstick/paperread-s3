#include "epub/XhtmlParser.h"
#include "utils/Log.h"
#include <pugixml.hpp>
#include <cstring>
#include <cctype>

static const char* TAG = "epub:xhtml";

namespace epub {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static BlockType heading_type_for_level(int level) {
    switch (level) {
        case 1: return BlockType::HEADING_1;
        case 2: return BlockType::HEADING_2;
        case 3: return BlockType::HEADING_3;
        case 4: return BlockType::HEADING_4;
        case 5: return BlockType::HEADING_5;
        case 6: return BlockType::HEADING_6;
        default: return BlockType::HEADING_1;
    }
}

/// Returns heading level 1-6 if tag is h1-h6, otherwise 0.
static int heading_level_from_tag(const char* tag) {
    if (tag[0] == 'h' && tag[1] >= '1' && tag[1] <= '6' && tag[2] == '\0') {
        return tag[1] - '0';
    }
    return 0;
}

static bool is_skip_tag(const char* tag) {
    return strcmp(tag, "head") == 0
        || strcmp(tag, "style") == 0
        || strcmp(tag, "script") == 0
        || strcmp(tag, "table") == 0;
}

// ---------------------------------------------------------------------------
// Parser state
// ---------------------------------------------------------------------------

struct ParserState {
    BlockList blocks;
    TextStyle current_style;
    bool need_new_block = true;

    void ensure_block(BlockType type = BlockType::PARAGRAPH, uint8_t hlevel = 0) {
        if (need_new_block || blocks.empty()) {
            blocks.push_back(Block{type, {}, {}, hlevel});
            need_new_block = false;
        }
    }

    void finish_block() {
        need_new_block = true;
    }

    void append_text(const std::string& text) {
        if (text.empty()) return;
        ensure_block();
        Block& blk = blocks.back();
        // Merge with previous run if same style
        if (!blk.runs.empty() && blk.runs.back().style == current_style) {
            blk.runs.back().text += text;
        } else {
            blk.runs.push_back(TextRun{text, current_style});
        }
    }
};

// ---------------------------------------------------------------------------
// Whitespace normalization
// ---------------------------------------------------------------------------

/// Collapse whitespace within a text node value: replace sequences of
/// whitespace with a single space. Preserves a leading/trailing space
/// if the original had one.
static std::string normalize_whitespace(const char* raw) {
    std::string out;
    out.reserve(strlen(raw));
    bool in_space = false;
    for (const char* p = raw; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f') {
            if (!in_space) {
                out += ' ';
                in_space = true;
            }
        } else {
            out += static_cast<char>(c);
            in_space = false;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Recursive tree walker
// ---------------------------------------------------------------------------

static void walk_node(const pugi::xml_node& node, ParserState& state);

static void walk_children(const pugi::xml_node& parent, ParserState& state) {
    for (pugi::xml_node child : parent.children()) {
        walk_node(child, state);
    }
}

static void walk_node(const pugi::xml_node& node, ParserState& state) {
    if (node.type() == pugi::node_pcdata || node.type() == pugi::node_cdata) {
        std::string text = normalize_whitespace(node.value());
        state.append_text(text);
        return;
    }

    if (node.type() != pugi::node_element) {
        return;
    }

    const char* tag = node.name();

    // Skip tags whose content we don't want
    if (is_skip_tag(tag)) {
        return;
    }

    // Image: create an IMAGE block
    if (strcmp(tag, "img") == 0) {
        const char* src = node.attribute("src").value();
        if (src && src[0] != '\0') {
            state.finish_block();
            state.blocks.push_back(Block{BlockType::IMAGE, {}, std::string(src), 0});
            state.finish_block();
        }
        return;
    }

    // Horizontal rule
    if (strcmp(tag, "hr") == 0) {
        state.finish_block();
        state.blocks.push_back(Block{BlockType::HORIZONTAL_RULE, {}, {}, 0});
        state.finish_block();
        return;
    }

    // Line break: start a new paragraph
    if (strcmp(tag, "br") == 0) {
        state.finish_block();
        return;
    }

    // Headings
    int hlevel = heading_level_from_tag(tag);
    if (hlevel > 0) {
        state.finish_block();
        state.ensure_block(heading_type_for_level(hlevel), static_cast<uint8_t>(hlevel));
        // Headings are bold by default
        TextStyle saved = state.current_style;
        state.current_style.bold = true;
        state.current_style.font_size_delta = static_cast<int8_t>(4 - hlevel + 1); // h1=+4, h6=-1
        walk_children(node, state);
        state.current_style = saved;
        state.finish_block();
        return;
    }

    // Block-level elements
    if (strcmp(tag, "p") == 0 || strcmp(tag, "div") == 0) {
        state.finish_block();
        state.ensure_block(BlockType::PARAGRAPH);
        walk_children(node, state);
        state.finish_block();
        return;
    }

    if (strcmp(tag, "blockquote") == 0) {
        state.finish_block();
        state.ensure_block(BlockType::BLOCKQUOTE);
        walk_children(node, state);
        state.finish_block();
        return;
    }

    if (strcmp(tag, "li") == 0) {
        state.finish_block();
        state.ensure_block(BlockType::LIST_ITEM);
        walk_children(node, state);
        state.finish_block();
        return;
    }

    // ul/ol: just recurse into children (li items handle their own blocks)
    if (strcmp(tag, "ul") == 0 || strcmp(tag, "ol") == 0) {
        walk_children(node, state);
        return;
    }

    // Inline formatting: bold
    if (strcmp(tag, "b") == 0 || strcmp(tag, "strong") == 0) {
        TextStyle saved = state.current_style;
        state.current_style.bold = true;
        walk_children(node, state);
        state.current_style = saved;
        return;
    }

    // Inline formatting: italic
    if (strcmp(tag, "i") == 0 || strcmp(tag, "em") == 0) {
        TextStyle saved = state.current_style;
        state.current_style.italic = true;
        walk_children(node, state);
        state.current_style = saved;
        return;
    }

    // Links
    if (strcmp(tag, "a") == 0) {
        TextStyle saved = state.current_style;
        state.current_style.is_link = true;
        walk_children(node, state);
        state.current_style = saved;
        return;
    }

    // sup/sub: could adjust font_size_delta but keep it simple
    if (strcmp(tag, "sup") == 0 || strcmp(tag, "sub") == 0) {
        TextStyle saved = state.current_style;
        state.current_style.font_size_delta = static_cast<int8_t>(
            state.current_style.font_size_delta - 2);
        walk_children(node, state);
        state.current_style = saved;
        return;
    }

    // Unknown/other inline tags (span, etc.): pass through content
    walk_children(node, state);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

BlockList XhtmlParser::parse(const char* xhtml_data, size_t length) {
    if (!xhtml_data || length == 0) {
        return {};
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(
        xhtml_data, length,
        pugi::parse_default | pugi::parse_ws_pcdata);

    if (!result) {
        LOG_W(TAG, "XML parse warning: %s (offset %zu)", result.description(),
              static_cast<size_t>(result.offset));
        // Even on error, pugixml often partially parses - proceed with what we have
    }

    // Find the body element, or fall back to document root
    pugi::xml_node root = doc.child("html");
    pugi::xml_node body = root ? root.child("body") : pugi::xml_node();
    pugi::xml_node start = body ? body : (root ? root : doc);

    ParserState state;
    walk_children(start, state);

    // Remove empty blocks (blocks with only whitespace)
    BlockList cleaned;
    cleaned.reserve(state.blocks.size());
    for (auto& blk : state.blocks) {
        if (!blk.is_empty()) {
            // Trim leading/trailing whitespace from first and last runs
            if (!blk.runs.empty()) {
                // Trim leading space from first run
                std::string& first = blk.runs.front().text;
                size_t start_pos = 0;
                while (start_pos < first.size() && first[start_pos] == ' ') {
                    start_pos++;
                }
                if (start_pos > 0) {
                    first.erase(0, start_pos);
                }
                // Trim trailing space from last run
                std::string& last = blk.runs.back().text;
                while (!last.empty() && last.back() == ' ') {
                    last.pop_back();
                }
                // Remove runs that became empty after trimming
                while (!blk.runs.empty() && blk.runs.front().text.empty()) {
                    blk.runs.erase(blk.runs.begin());
                }
                while (!blk.runs.empty() && blk.runs.back().text.empty()) {
                    blk.runs.pop_back();
                }
            }
            // Re-check if block is still non-empty after trimming
            if (!blk.is_empty()) {
                cleaned.push_back(std::move(blk));
            }
        }
    }

    return cleaned;
}

} // namespace epub
