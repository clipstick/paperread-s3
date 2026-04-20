#pragma once

#include "BookModel.h"

class ZipReader;

/**
 * OpfParser - Parse EPUB packaging documents.
 *
 * Reads META-INF/container.xml to locate the OPF, then parses
 * metadata, manifest, and spine. If an NCX or EPUB3 nav document
 * is referenced, its table of contents is parsed as well.
 *
 * All XML parsing uses pugixml.
 */
namespace OpfParser {

/**
 * Parse an EPUB file into a BookModel.
 *
 * @param zip   An open ZipReader for the EPUB file
 * @param out   Receives the parsed book data on success
 * @return true on success, false on unrecoverable error
 */
bool parse(ZipReader& zip, BookModel& out);

} // namespace OpfParser
