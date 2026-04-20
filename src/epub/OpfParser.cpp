#include "epub/OpfParser.h"
#include "epub/ZipReader.h"
#include "utils/Log.h"

#include <cstring>
#include <map>
#include <string>
#include <pugixml.hpp>

static const char* TAG = "epub:opf";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Normalise a path by collapsing ".." components. */
static std::string normalise_path(const std::string& path)
{
    std::vector<std::string> parts;
    std::string part;
    for (char c : path) {
        if (c == '/') {
            if (part == "..") {
                if (!parts.empty()) parts.pop_back();
            } else if (!part.empty() && part != ".") {
                parts.push_back(part);
            }
            part.clear();
        } else {
            part += c;
        }
    }
    if (part == "..") {
        if (!parts.empty()) parts.pop_back();
    } else if (!part.empty() && part != ".") {
        parts.push_back(part);
    }

    std::string result;
    for (auto& p : parts) {
        if (!result.empty()) result += '/';
        result += p;
    }
    return result;
}

/** Extract directory prefix from a path (includes trailing slash). */
static std::string dir_of(const std::string& path)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos) return "";
    return path.substr(0, pos + 1);
}

/** Split href on '#' into path and anchor. */
static void split_fragment(const std::string& href,
                           std::string& path_out,
                           std::string& anchor_out)
{
    auto pos = href.find('#');
    if (pos != std::string::npos) {
        path_out = href.substr(0, pos);
        anchor_out = href.substr(pos + 1);
    } else {
        path_out = href;
        anchor_out.clear();
    }
}

/** Load an XML buffer via pugixml; returns false on parse error. */
static bool load_xml(pugi::xml_document& doc, const uint8_t* data, size_t size)
{
    pugi::xml_parse_result res = doc.load_buffer(data, size);
    if (!res) {
        LOG_E(TAG, "XML parse error: %s", res.description());
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// container.xml
// ---------------------------------------------------------------------------

static bool find_opf_path(ZipReader& zip, std::string& opf_path)
{
    size_t size = 0;
    uint8_t* data = zip.read_entry("META-INF/container.xml", &size);
    if (!data) {
        LOG_E(TAG, "Missing META-INF/container.xml");
        return false;
    }

    pugi::xml_document doc;
    bool ok = load_xml(doc, data, size);
    free(data);
    if (!ok) return false;

    pugi::xml_node container = doc.child("container");
    if (!container) {
        LOG_E(TAG, "No <container> element");
        return false;
    }

    for (pugi::xml_node rf = container.child("rootfiles").child("rootfile");
         rf; rf = rf.next_sibling("rootfile"))
    {
        const char* mt = rf.attribute("media-type").value();
        if (mt && strcmp(mt, "application/oebps-package+xml") == 0) {
            const char* fp = rf.attribute("full-path").value();
            if (fp && fp[0] != '\0') {
                opf_path = fp;
                return true;
            }
        }
    }

    LOG_E(TAG, "No OPF rootfile found in container.xml");
    return false;
}

// ---------------------------------------------------------------------------
// OPF (metadata + manifest + spine)
// ---------------------------------------------------------------------------

static bool parse_opf(ZipReader& zip, const std::string& opf_path, BookModel& out)
{
    out.base_path = dir_of(opf_path);

    size_t size = 0;
    uint8_t* data = zip.read_entry(opf_path.c_str(), &size);
    if (!data) {
        LOG_E(TAG, "Failed to read OPF: %s", opf_path.c_str());
        return false;
    }

    pugi::xml_document doc;
    bool ok = load_xml(doc, data, size);
    free(data);
    if (!ok) return false;

    pugi::xml_node package = doc.child("package");
    if (!package) {
        LOG_E(TAG, "No <package> element in OPF");
        return false;
    }

    // ---- metadata ----
    pugi::xml_node meta = package.child("metadata");
    if (meta) {
        pugi::xml_node t = meta.child("dc:title");
        if (t) out.metadata.title = t.child_value();

        pugi::xml_node a = meta.child("dc:creator");
        if (a) out.metadata.author = a.child_value();

        pugi::xml_node l = meta.child("dc:language");
        if (l) out.metadata.language = l.child_value();
    }
    if (out.metadata.title.empty()) {
        LOG_W(TAG, "OPF has no title");
    }

    // Determine cover id from <meta name="cover" content="...">
    const char* cover_id = nullptr;
    if (meta) {
        pugi::xml_node cover_meta = meta.find_child_by_attribute("meta", "name", "cover");
        if (cover_meta) {
            cover_id = cover_meta.attribute("content").value();
            if (cover_id && cover_id[0] == '\0') cover_id = nullptr;
        }
    }

    // ---- manifest ----
    pugi::xml_node manifest = package.child("manifest");
    if (!manifest) {
        LOG_E(TAG, "No <manifest> in OPF");
        return false;
    }

    // Map id -> href (full path), also track NCX/nav items
    std::map<std::string, std::string> id_to_href;
    std::string ncx_href;
    std::string nav_href;

    for (pugi::xml_node item = manifest.child("item"); item; item = item.next_sibling("item"))
    {
        const char* id_attr = item.attribute("id").value();
        const char* href_attr = item.attribute("href").value();
        if (!id_attr || !href_attr || id_attr[0] == '\0' || href_attr[0] == '\0')
            continue;

        std::string full_href = normalise_path(out.base_path + href_attr);
        id_to_href[id_attr] = full_href;

        // Detect cover image
        if (cover_id && strcmp(id_attr, cover_id) == 0) {
            out.metadata.cover_href = full_href;
        }

        // Also check for properties="cover-image" (EPUB3)
        const char* props = item.attribute("properties").value();
        if (props && strstr(props, "cover-image")) {
            out.metadata.cover_href = full_href;
        }

        // Detect NCX
        const char* media_type = item.attribute("media-type").value();
        if (media_type && strcmp(media_type, "application/x-dtbncx+xml") == 0) {
            ncx_href = full_href;
        }

        // Detect EPUB3 nav
        if (props && strstr(props, "nav")) {
            nav_href = full_href;
        }
    }

    // ---- spine ----
    pugi::xml_node spine = package.child("spine");
    if (!spine) {
        LOG_E(TAG, "No <spine> in OPF");
        return false;
    }

    // Check spine toc attribute for NCX fallback
    if (ncx_href.empty()) {
        const char* toc_attr = spine.attribute("toc").value();
        if (toc_attr && toc_attr[0] != '\0') {
            auto it = id_to_href.find(toc_attr);
            if (it != id_to_href.end()) {
                ncx_href = it->second;
            }
        }
    }

    for (pugi::xml_node itemref = spine.child("itemref"); itemref;
         itemref = itemref.next_sibling("itemref"))
    {
        const char* idref = itemref.attribute("idref").value();
        if (!idref || idref[0] == '\0') continue;

        auto it = id_to_href.find(idref);
        if (it != id_to_href.end()) {
            SpineItem si;
            si.id = idref;
            si.href = it->second;
            out.spine.push_back(si);
        }
    }

    LOG_I(TAG, "OPF: title='%s', spine=%d items",
          out.metadata.title.c_str(), (int)out.spine.size());

    // ---- TOC ----
    // Prefer EPUB3 nav, fall back to NCX
    if (!nav_href.empty()) {
        // parse_nav is defined below; forward-declared via the namespace-internal
        // linkage. We attempt nav first and fall back to NCX on failure.
        // (implemented in the nav parsing section below)
    }

    return true;
}

// ---------------------------------------------------------------------------
// NCX table of contents
// ---------------------------------------------------------------------------

static void parse_ncx_navpoints(pugi::xml_node parent,
                                const std::string& base_path,
                                std::vector<TocItem>& items)
{
    for (pugi::xml_node np = parent.child("navPoint"); np; np = np.next_sibling("navPoint"))
    {
        const char* label = nullptr;
        pugi::xml_node text_node = np.child("navLabel").child("text");
        if (text_node) label = text_node.child_value();
        if (!label || label[0] == '\0') continue;

        const char* src = np.child("content").attribute("src").value();
        if (!src || src[0] == '\0') continue;

        TocItem ti;
        ti.title = label;

        std::string full = normalise_path(base_path + src);
        split_fragment(full, ti.href, ti.anchor);

        // Recurse into nested navPoints
        parse_ncx_navpoints(np, base_path, ti.children);

        items.push_back(std::move(ti));
    }
}

static bool parse_ncx(ZipReader& zip, const std::string& ncx_href,
                       const std::string& base_path, std::vector<TocItem>& toc)
{
    size_t size = 0;
    uint8_t* data = zip.read_entry(ncx_href.c_str(), &size);
    if (!data) {
        LOG_W(TAG, "Failed to read NCX: %s", ncx_href.c_str());
        return false;
    }

    pugi::xml_document doc;
    bool ok = load_xml(doc, data, size);
    free(data);
    if (!ok) return false;

    pugi::xml_node ncx = doc.child("ncx");
    if (!ncx) {
        LOG_W(TAG, "No <ncx> element in NCX file");
        return false;
    }

    pugi::xml_node nav_map = ncx.child("navMap");
    if (!nav_map) {
        LOG_W(TAG, "No <navMap> in NCX");
        return false;
    }

    // NCX hrefs are relative to the NCX file itself
    std::string ncx_base = dir_of(ncx_href);
    parse_ncx_navpoints(nav_map, ncx_base, toc);

    LOG_I(TAG, "NCX: %d top-level TOC entries", (int)toc.size());
    return true;
}

// ---------------------------------------------------------------------------
// EPUB3 nav document
// ---------------------------------------------------------------------------

static void parse_nav_ol(pugi::xml_node ol_node,
                         const std::string& base_path,
                         std::vector<TocItem>& items)
{
    for (pugi::xml_node li = ol_node.child("li"); li; li = li.next_sibling("li"))
    {
        pugi::xml_node a = li.child("a");
        if (!a) continue;

        const char* label = a.child_value();
        if (!label || label[0] == '\0') continue;

        const char* href_attr = a.attribute("href").value();
        if (!href_attr || href_attr[0] == '\0') continue;

        TocItem ti;
        ti.title = label;

        std::string full = normalise_path(base_path + href_attr);
        split_fragment(full, ti.href, ti.anchor);

        // Recurse into nested <ol>
        pugi::xml_node nested_ol = li.child("ol");
        if (nested_ol) {
            parse_nav_ol(nested_ol, base_path, ti.children);
        }

        items.push_back(std::move(ti));
    }
}

static bool parse_nav(ZipReader& zip, const std::string& nav_href,
                      std::vector<TocItem>& toc)
{
    size_t size = 0;
    uint8_t* data = zip.read_entry(nav_href.c_str(), &size);
    if (!data) {
        LOG_W(TAG, "Failed to read nav: %s", nav_href.c_str());
        return false;
    }

    pugi::xml_document doc;
    bool ok = load_xml(doc, data, size);
    free(data);
    if (!ok) return false;

    // The nav document is XHTML; the <nav> element may be inside <html><body>
    // Walk all descendants looking for <nav epub:type="toc"> or just the first <nav>
    std::string nav_base = dir_of(nav_href);
    pugi::xml_node nav_node;

    // Try to find nav with epub:type="toc" first
    nav_node = doc.find_node([](pugi::xml_node n) {
        if (strcmp(n.name(), "nav") != 0) return false;
        const char* epub_type = n.attribute("epub:type").value();
        return epub_type && strstr(epub_type, "toc") != nullptr;
    });

    // Fall back to first <nav> element
    if (!nav_node) {
        nav_node = doc.find_node([](pugi::xml_node n) {
            return strcmp(n.name(), "nav") == 0;
        });
    }

    if (!nav_node) {
        LOG_W(TAG, "No <nav> element in nav document");
        return false;
    }

    pugi::xml_node ol = nav_node.child("ol");
    if (!ol) {
        LOG_W(TAG, "No <ol> in nav element");
        return false;
    }

    parse_nav_ol(ol, nav_base, toc);
    LOG_I(TAG, "Nav: %d top-level TOC entries", (int)toc.size());
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool OpfParser::parse(ZipReader& zip, BookModel& out)
{
    if (!zip.is_open()) {
        LOG_E(TAG, "ZipReader is not open");
        return false;
    }

    // Step 1: find OPF path from container.xml
    std::string opf_path;
    if (!find_opf_path(zip, opf_path)) {
        return false;
    }
    LOG_I(TAG, "OPF path: %s", opf_path.c_str());

    // Step 2: parse OPF (metadata, manifest, spine)
    // We need the NCX/nav href from inside parse_opf, so we extract
    // them by re-reading manifest. For simplicity, parse_opf fills out
    // the model, and we do the TOC in a second pass.
    if (!parse_opf(zip, opf_path, out)) {
        return false;
    }

    // Step 3: parse TOC (need to re-read manifest to find NCX/nav hrefs)
    // Re-extract from OPF to get the TOC href
    {
        size_t size = 0;
        uint8_t* data = zip.read_entry(opf_path.c_str(), &size);
        if (data) {
            pugi::xml_document doc;
            if (load_xml(doc, data, size)) {
                pugi::xml_node package = doc.child("package");
                pugi::xml_node manifest = package.child("manifest");
                pugi::xml_node spine_node = package.child("spine");

                std::string ncx_href;
                std::string nav_href;

                for (pugi::xml_node item = manifest.child("item"); item;
                     item = item.next_sibling("item"))
                {
                    const char* mt = item.attribute("media-type").value();
                    const char* props = item.attribute("properties").value();
                    const char* href_attr = item.attribute("href").value();
                    if (!href_attr || href_attr[0] == '\0') continue;

                    std::string full = normalise_path(out.base_path + href_attr);

                    if (mt && strcmp(mt, "application/x-dtbncx+xml") == 0) {
                        ncx_href = full;
                    }
                    if (props && strstr(props, "nav")) {
                        nav_href = full;
                    }
                }

                // Also check spine toc attribute
                if (ncx_href.empty() && spine_node) {
                    const char* toc_attr = spine_node.attribute("toc").value();
                    if (toc_attr && toc_attr[0] != '\0') {
                        for (pugi::xml_node item = manifest.child("item"); item;
                             item = item.next_sibling("item"))
                        {
                            const char* id_attr = item.attribute("id").value();
                            const char* href_attr = item.attribute("href").value();
                            if (id_attr && href_attr && strcmp(id_attr, toc_attr) == 0) {
                                ncx_href = normalise_path(out.base_path + href_attr);
                                break;
                            }
                        }
                    }
                }

                // Try EPUB3 nav first, fall back to NCX
                bool toc_ok = false;
                if (!nav_href.empty()) {
                    toc_ok = parse_nav(zip, nav_href, out.toc);
                }
                if (!toc_ok && !ncx_href.empty()) {
                    toc_ok = parse_ncx(zip, ncx_href, out.base_path, out.toc);
                }
                if (!toc_ok) {
                    LOG_W(TAG, "No TOC found (continuing without)");
                }
            }
            free(data);
        }
    }

    LOG_I(TAG, "Parsed EPUB: '%s' by '%s', %d spine items, %d TOC entries",
          out.metadata.title.c_str(),
          out.metadata.author.c_str(),
          (int)out.spine.size(),
          (int)out.toc.size());

    return true;
}
