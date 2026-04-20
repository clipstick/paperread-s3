#pragma once

#include <cstdint>
#include <string>

/**
 * Hash Utilities for Cache Keys
 *
 * Uses xxHash64 for fast, high-quality hashing of file paths and contents.
 * Hash values are used to generate stable cache directory names for EPUB
 * metadata, layouts, and cover images.
 *
 * Usage:
 *   std::string epub_path = "/fs/Books/mybook.epub";
 *   uint64_t hash = hash_file_path(epub_path);
 *   std::string cache_dir = "/fs/.paperread/epub_" + hash_to_hex(hash);
 *
 * Cache directory layout:
 *   /fs/.paperread/epub_<hash>/
 *     book.bin           - Serialized BookModel (metadata, spine, TOC)
 *     cover.jpg          - Extracted cover image
 *     sections/          - Layout cache per chapter
 *       0.bin, 1.bin, ...
 *     progress.bin       - Reading progress
 */

/**
 * Hash a file path to generate a stable cache key
 * @param path Absolute path to file (e.g., "/fs/Books/mybook.epub")
 * @return 64-bit hash value
 */
uint64_t hash_file_path(const std::string& path);

/**
 * Hash arbitrary data
 * @param data Pointer to data
 * @param size Size of data in bytes
 * @return 64-bit hash value
 */
uint64_t hash_data(const void* data, size_t size);

/**
 * Convert hash to hex string for directory names
 * @param hash Hash value
 * @return Lowercase hex string (16 characters)
 */
std::string hash_to_hex(uint64_t hash);

/**
 * Generate full cache directory path for an EPUB file
 * @param epub_path Path to EPUB file
 * @return Cache directory path (e.g., "/fs/.paperread/epub_0123456789abcdef")
 */
std::string get_cache_dir_for_epub(const std::string& epub_path);
