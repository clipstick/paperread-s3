#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

/**
 * ZipReader - Read entries from an EPUB (ZIP) file on disk.
 *
 * Wraps miniz to provide a simple interface for extracting individual
 * files from a ZIP archive. The archive is opened on construction and
 * closed on destruction; individual entries are extracted on demand.
 */
class ZipReader {
public:
    explicit ZipReader(const std::string& path);
    ~ZipReader();

    // Non-copyable, non-movable
    ZipReader(const ZipReader&) = delete;
    ZipReader& operator=(const ZipReader&) = delete;

    /** @return true if the archive was opened successfully */
    bool is_open() const { return m_open; }

    /**
     * Extract a file from the archive into a newly allocated buffer.
     * The buffer is null-terminated (one extra byte) for convenient
     * use as a C string with XML/HTML content.
     *
     * Caller must free() the returned pointer.
     *
     * @param entry_path  Path inside the ZIP (e.g. "META-INF/container.xml")
     * @param out_size    If non-null, receives the uncompressed size (excluding null terminator)
     * @return Pointer to the buffer, or nullptr on failure
     */
    uint8_t* read_entry(const char* entry_path, size_t* out_size = nullptr);

private:
    std::string m_path;
    bool m_open;
    void* m_archive;   // mz_zip_archive*, opaque to avoid leaking miniz into headers
};
