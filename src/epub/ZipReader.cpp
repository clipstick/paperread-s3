#include "epub/ZipReader.h"
#include "utils/Log.h"

#include <cstring>
#include <cstdlib>

#ifndef UNIT_TEST
#if defined(BOARD_HAS_PSRAM)
#include <esp_heap_caps.h>
#endif
#endif

// Use full miniz 3.1.0 (not ESP-IDF's stripped ROM version)
#include "epub/miniz_full.h"

static const char* TAG = "epub:zip";

ZipReader::ZipReader(const std::string& path)
    : m_path(path), m_open(false), m_archive(nullptr)
{
    auto* archive = static_cast<mz_zip_archive*>(calloc(1, sizeof(mz_zip_archive)));
    if (!archive) {
        LOG_E(TAG, "Failed to allocate mz_zip_archive");
        return;
    }

    if (!mz_zip_reader_init_file(archive, m_path.c_str(), 0)) {
        LOG_E(TAG, "Failed to open ZIP: %s", m_path.c_str());
        free(archive);
        return;
    }

    m_archive = archive;
    m_open = true;
    LOG_D(TAG, "Opened ZIP: %s", m_path.c_str());
}

ZipReader::~ZipReader()
{
    if (m_archive) {
        auto* archive = static_cast<mz_zip_archive*>(m_archive);
        if (m_open) {
            mz_zip_reader_end(archive);
        }
        free(archive);
        m_archive = nullptr;
    }
}

uint8_t* ZipReader::read_entry(const char* entry_path, size_t* out_size)
{
    if (!m_open || !m_archive) {
        LOG_E(TAG, "read_entry called on closed archive");
        return nullptr;
    }

    auto* archive = static_cast<mz_zip_archive*>(m_archive);

    mz_uint32 file_index = 0;
    if (!mz_zip_reader_locate_file_v2(archive, entry_path, nullptr, 0, &file_index)) {
        LOG_E(TAG, "Entry not found: %s", entry_path);
        return nullptr;
    }

    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(archive, file_index, &file_stat)) {
        LOG_E(TAG, "Failed to stat entry: %s", entry_path);
        return nullptr;
    }

    size_t file_size = static_cast<size_t>(file_stat.m_uncomp_size);

    // Allocate in PSRAM when available (with +1 for null terminator)
    uint8_t* buf;
#if !defined(UNIT_TEST) && defined(BOARD_HAS_PSRAM)
    buf = static_cast<uint8_t*>(
        heap_caps_calloc(file_size + 1, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
    buf = static_cast<uint8_t*>(calloc(file_size + 1, 1));
#endif

    if (!buf) {
        LOG_E(TAG, "Alloc failed for %s (%zu bytes)", entry_path, file_size);
        return nullptr;
    }

    if (!mz_zip_reader_extract_to_mem(archive, file_index, buf, file_size, 0)) {
        LOG_E(TAG, "Extract failed: %s", entry_path);
        free(buf);
        return nullptr;
    }

    if (out_size) {
        *out_size = file_size;
    }

    LOG_D(TAG, "Extracted %s (%zu bytes)", entry_path, file_size);
    return buf;
}
