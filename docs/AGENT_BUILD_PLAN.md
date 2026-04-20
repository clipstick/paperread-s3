# PaperRead S3 — Claude Code Agent Build Plan

> **Purpose:** Structured build plan for Claude Code and agent teams. Each phase contains discrete tasks with clear context, instructions, acceptance criteria, and dependencies. Work phases sequentially; tasks within a phase can be parallelised where noted.

---

## How to Use This Document

**For the orchestrator (human or lead agent):**
- Phases are sequential — don't start Phase N+1 until Phase N has passed human review
- Tasks marked `parallel` within a phase can be delegated to separate agents simultaneously
- Tasks marked `sequential` must be done in order within the phase
- Each task is self-contained enough to hand to a sub-agent with minimal additional context

**For sub-agents executing a task:**
- Read the full task spec before starting
- Follow the conventions in "Global Conventions" below — they apply to every task
- Produce the artefacts listed under "Deliverables"
- Verify all "Acceptance Criteria" before marking the task complete
- If blocked, write findings to `docs/blockers/<task-id>.md` instead of guessing

**Recommended agent team roles:**
- **Architect** — reviews designs, maintains architectural consistency, owns cross-cutting concerns
- **Firmware Engineer (HAL)** — hardware abstraction, drivers, low-level I/O
- **Firmware Engineer (Core)** — EPUB parsing, layout engine, rendering
- **Firmware Engineer (UX)** — UI activities, touch input, navigation
- **Firmware Engineer (Network)** — WiFi, HTTP, OTA, sync
- **QA Engineer** — test fixtures, integration testing, regression

---

## Project Overview

**Target:** M5Stack Paper S3 (ESP32-S3R8, 8 MB PSRAM, 16 MB flash, 960×540 e-ink, GT911 touch)
**Build system:** PlatformIO + ESP-IDF framework
**Language:** C / C++20
**License:** MIT

**Primary reference repos:**
| Repo | Purpose |
|---|---|
| `juicecultus/diy-esp32s3-epub-reader` | Working Paper S3 base (epdiy + touch + FreeType) |
| `crosspoint-reader/crosspoint-reader` | Feature and UX reference |
| `erikbotta/erbopubreader` (source via erbosoft.org) | Dark mode, USB disk, NTP, alarms reference |
| `atomic14/diy-esp32-epub-reader` | ZIP + XML parsing reference |
| `vroland/epdiy` | E-ink panel driver |

---

## Global Conventions

All agents must follow these throughout the project.

### Code Style
- C++20, clang-format enforced (copy `.clang-format` from CrossPoint repo)
- Headers in `include/`, mirroring `src/` structure
- Business logic never calls hardware libraries directly — always go through `hal/` interfaces
- One logging macro family: `LOG_D/I/W/E(tag, fmt, ...)` — never raw `printf` or `ESP_LOGx`

### Memory Rules
- Any heap allocation > 4 KB uses PSRAM: `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`
- Framebuffers always in PSRAM
- Internal RAM reserved for stack, ISRs, DMA buffers
- Use RAII wrappers (`PsramBuffer<T>`) rather than raw allocation where possible

### File System Layout (SD card)
```
/fs/
├── Books/                          # user's EPUB files (nested folders allowed)
├── fonts/
│   └── reader.ttf                  # optional user font
└── .paperread/
    ├── settings.json               # user configuration
    └── epub_<xxhash>/              # per-book cache
        ├── book.bin                # metadata + spine + TOC
        ├── cover.jpg               # extracted cover
        ├── progress.bin            # current + furthest-read position
        └── sections/
            └── <n>.bin             # laid-out pages per chapter
```

### Git Discipline
- Commit format: `[component] Short description` e.g. `[epub] Fix OPF spine ordering for EPUB3`
- One logical change per commit
- Never push a broken build to main
- Every task ends with a commit and (if applicable) a PR

### Testing
- Host-testable logic (parsers, layout, settings) lives in `test/unit/` using PlatformIO native test
- Hardware-dependent tests live in `test/integration/`
- Target coverage: 70%+ on parser and layout code

### Documentation
- Every public header has a top-of-file doc comment explaining its purpose
- Every `hal/` interface has a usage example in its header
- User-facing docs go in `docs/` (USER_GUIDE.md, FLASHING.md, etc.)

---

## Phase 0 — Repository Bootstrap

**Must be done by a human before agent work begins.**

1. Create the GitHub repository: `paperread-s3`
2. Clone juicecultus fork as starting point:
   ```bash
   git clone --recursive https://github.com/juicecultus/diy-esp32s3-epub-reader paperread-s3
   cd paperread-s3
   git remote rename origin upstream
   git remote add origin <your-new-repo-url>
   git push -u origin main
   ```
3. Verify `pio run -e paper_s3_idf` builds clean on your machine
4. Flash to Paper S3 and confirm the existing demo boots, shows library, and renders a test EPUB
5. Add the PRD to `docs/PRD.md`
6. Add this file as `docs/AGENT_BUILD_PLAN.md`
7. Create a GitHub Project board with columns: Backlog, In Progress, Review, Done

**Acceptance:** Device boots existing demo firmware successfully. Repo is ready to receive agent work.

---

## Phase 1 — Scaffolding & HAL

**Goal:** Restructure the codebase and establish a clean hardware abstraction layer so all subsequent work is hardware-agnostic.
**Duration estimate:** 2–3 agent sessions
**Human review required:** Yes — verify flash + boot at end of phase

### Task 1.1 — Directory Restructure `[sequential]`

**Owner:** Architect
**Depends on:** Phase 0

**Context:**
The juicecultus fork has a working Paper S3 build but its layout was inherited from atomic14's original. Restructure to the target layout from the PRD before adding new code.

**Instructions:**
1. Reorganise sources to this structure:
   ```
   src/
     main/          # app entry, FreeRTOS tasks, boot sequence
     hal/           # concrete HAL implementations
     epub/          # ZIP, OPF, HTML parser, layout engine
     ui/            # activity system: library, reader, settings, sleep
     fonts/         # FreeType integration, glyph cache
     net/           # WiFi, HTTP server, OTA, KOReader sync
     storage/       # settings, metadata cache, progress persistence
     utils/         # logging, memory helpers, QR code, hash
   include/
     hal/           # HAL interfaces (abstract)
     epub/
     ui/
     fonts/
     net/
     storage/
     utils/
   test/
     unit/          # PlatformIO native test (host-side)
     integration/   # on-device tests
     fixtures/      # test EPUBs, images, XML
   components/
     epdiy/         # ESP-IDF component (keep as submodule)
   externals/       # vendored single-header libs (xxhash, etc.)
   docs/            # PRD, AGENT_BUILD_PLAN, USER_GUIDE, FLASHING
   ```
2. Update all `#include` paths, `CMakeLists.txt`, and `idf_component.yml` files
3. Preserve git history using `git mv` where possible
4. Verify `pio run -e paper_s3_idf` still compiles cleanly
5. Flash and verify behaviour is unchanged from Phase 0

**Deliverables:**
- Restructured repository
- Updated build files
- Commit: `[repo] Restructure to PaperRead layout`

**Acceptance Criteria:**
- `pio run -e paper_s3_idf` builds with zero errors
- Device boots and renders EPUB as it did after Phase 0
- No dead includes or orphaned files

---

### Task 1.2 — Logging and Memory Utilities `[sequential]`

**Owner:** Architect
**Depends on:** 1.1

**Context:** Every other task needs logging and PSRAM helpers. Build these first.

**Instructions:**

Create `include/utils/Log.h`:
```cpp
#pragma once
#include "esp_log.h"

#define LOG_D(tag, fmt, ...) ESP_LOGD(tag, fmt, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#define LOG_E(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
```

Create `include/utils/Memory.h` and `src/utils/Memory.cpp`:
```cpp
void* psram_malloc(size_t size);
void* psram_calloc(size_t count, size_t size);
void  psram_free(void* ptr);

template<typename T>
class PsramBuffer {
public:
    explicit PsramBuffer(size_t count);
    ~PsramBuffer();
    PsramBuffer(const PsramBuffer&) = delete;
    PsramBuffer& operator=(const PsramBuffer&) = delete;
    PsramBuffer(PsramBuffer&&) noexcept;
    PsramBuffer& operator=(PsramBuffer&&) noexcept;

    T* data() { return _buf; }
    const T* data() const { return _buf; }
    size_t size() const { return _size; }
    T& operator[](size_t i) { return _buf[i]; }

private:
    T* _buf = nullptr;
    size_t _size = 0;
};
```

Create `include/utils/Hash.h`:
- Vendor `xxhash.h` single-header into `externals/`
- Expose `uint32_t xxhash32(const void* data, size_t len, uint32_t seed = 0)`
- Expose `std::string bookCacheKey(const std::string& filePath)` returning `"epub_<hex>"`

**Deliverables:**
- All three utility headers + implementations
- Unit tests in `test/unit/test_memory.cpp` and `test/unit/test_hash.cpp`
- Commit: `[utils] Add logging, PSRAM, and hash utilities`

**Acceptance Criteria:**
- Unit tests pass: `pio test -e native`
- `PsramBuffer<uint8_t>` of 1 MB allocates successfully on device
- `xxhash32` produces stable values across runs

---

### Task 1.3 — HAL Interfaces `[sequential]`

**Owner:** Architect
**Depends on:** 1.2

**Context:** Define abstract interfaces so no business logic ever depends on epdiy, GT911, BMI270, etc. directly.

**Instructions:**

Create the following headers in `include/hal/`:

```cpp
// include/hal/IDisplay.h
#pragma once
enum class RefreshMode { FAST_A2, FAST_DU, BALANCED_GL16, QUALITY_GC16 };

class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual void init() = 0;
    virtual void flush(const uint8_t* framebuffer, RefreshMode mode) = 0;
    virtual void clear() = 0;
    virtual int width() const = 0;       // logical width (540)
    virtual int height() const = 0;      // logical height (960)
    virtual int bitsPerPixel() const = 0; // 1 for text fast path, 4 for grayscale
    virtual void sleep() = 0;
    virtual void wake() = 0;
};
```

```cpp
// include/hal/ITouch.h
#pragma once
struct TouchPoint { int x; int y; };

class ITouch {
public:
    virtual ~ITouch() = default;
    virtual void init() = 0;
    // Returns number of active touch points (0, 1, or 2). Fills out[].
    virtual int read(TouchPoint out[2]) = 0;
};
```

```cpp
// include/hal/IBattery.h
#pragma once
class IBattery {
public:
    virtual ~IBattery() = default;
    virtual void init() = 0;
    virtual int levelPercent() = 0;
    virtual float voltage() = 0;
    virtual bool isCharging() = 0;
};
```

```cpp
// include/hal/IIMU.h
#pragma once
enum class Orientation { PORTRAIT, LANDSCAPE_LEFT, LANDSCAPE_RIGHT, PORTRAIT_INVERTED };

class IIMU {
public:
    virtual ~IIMU() = default;
    virtual void init() = 0;
    virtual Orientation orientation() = 0;
    virtual bool hasBeenLifted() = 0;   // latched event
    virtual void clearLiftEvent() = 0;
};
```

```cpp
// include/hal/IStorage.h
#pragma once
class IStorage {
public:
    virtual ~IStorage() = default;
    virtual bool init() = 0;
    virtual bool isMounted() = 0;
    virtual uint64_t freeBytes() = 0;
    virtual uint64_t totalBytes() = 0;
    virtual const char* rootPath() = 0;  // typically "/fs"
};
```

```cpp
// include/hal/IRtc.h
#pragma once
#include <ctime>
class IRtc {
public:
    virtual ~IRtc() = default;
    virtual void init() = 0;
    virtual time_t now() = 0;
    virtual void set(time_t t) = 0;
    virtual void setWakeAlarm(time_t wakeAt) = 0;
    virtual bool wokeFromAlarm() = 0;
};
```

Each header must include a usage example in its top comment block.

**Deliverables:**
- All HAL headers
- `include/hal/Board.h` declaring `struct Board { IDisplay*; ITouch*; IBattery*; IIMU*; IStorage*; IRtc*; };`
- `include/hal/BoardFactory.h` with `Board& boardInstance();`
- Commit: `[hal] Define hardware abstraction interfaces`

**Acceptance Criteria:**
- All headers compile standalone
- No concrete implementations yet — interfaces only
- Architect approval before proceeding

---

### Task 1.4 — HAL Implementations `[parallel — split by driver]`

**Owner:** Firmware Engineer (HAL)
**Depends on:** 1.3
**Parallelisation:** Split into 6 sub-tasks, one per driver. Agents can work concurrently.

**Context:** Implement each HAL interface using the Paper S3's actual hardware.

**Sub-tasks:**

**1.4a — Display (`EpdiyDisplay`):**
- Wrap `vroland/epdiy`
- Use the working config from juicecultus fork's `sdkconfig.paper_s3_idf`
- Map `RefreshMode` enum to epdiy waveform modes (A2, DU, GL16, GC16)
- Handle portrait (540×960 logical) with rotation applied at flush time
- Allocate framebuffer in PSRAM

**1.4b — Touch (`Gt911Touch`):**
- Use ESP-IDF I2C master driver
- GT911 at I2C address 0x5D (or 0x14 depending on unit)
- Return coordinates in the logical 540×960 space
- Handle rotation if display is rotated

**1.4c — Battery (`AdcBattery`):**
- Read battery voltage via ADC on the appropriate GPIO (verify pin from Paper S3 schematic)
- Convert to percentage using a LiPo discharge curve (11-sample median filter for smoothing — reference erbopubreader)
- Detect charging state from USB power presence

**1.4d — IMU (`Bmi270Imu`):**
- Use M5Unified's BMI270 wrapper if permissively licensed, otherwise direct I2C
- Derive orientation from gravity vector
- Implement lift-detection by monitoring acceleration spikes

**1.4e — Storage (`SdStorage`):**
- Wrap `esp_vfs_fat` SD SPI mount
- Mount point `/fs`
- SPI pins from Paper S3 pinout (verify against M5Stack schematic)

**1.4f — RTC (`Bm8563Rtc`):**
- BM8563 on I2C
- Implement wake alarm using deep sleep + RTC interrupt

**Deliverables per sub-task:**
- `src/hal/<Name>.cpp` and corresponding header if needed for concrete state
- A smoke test in `test/integration/` that exercises the driver on device
- Commit: `[hal] Implement <driver name>`

**Acceptance Criteria:**
- All six drivers compile and link
- `BoardFactory::boardInstance()` returns a fully populated Board
- On-device smoke test: init all drivers, print status, no crashes

---

## Phase 2 — EPUB Parsing

**Goal:** Parse any EPUB 2 or EPUB 3 file into an in-memory book model ready for layout.
**Duration estimate:** 3–4 agent sessions
**Parallelisation:** Tasks 2.1 and 2.2 can run in parallel; 2.3 depends on both.

### Task 2.1 — ZIP + OPF Parser `[parallel with 2.2]`

**Owner:** Firmware Engineer (Core)
**Depends on:** Phase 1

**Context:** EPUB files are ZIP archives. The juicecultus fork already uses `miniz-esp32`. The OPF file describes the book's spine and metadata.

**Reference code:** `lib/Epub/` in juicecultus fork

**Instructions:**

Create `include/epub/ZipReader.h` + `src/epub/ZipReader.cpp`:
```cpp
class ZipReader {
public:
    bool open(const std::string& path);
    void close();
    bool listEntries(std::vector<std::string>& out);
    bool extractToBuffer(const std::string& entry, PsramBuffer<uint8_t>& out);
    bool extractToFile(const std::string& entry, const std::string& destPath);
    bool exists(const std::string& entry);
};
```

Create `include/epub/BookModel.h`:
```cpp
struct BookMetadata {
    std::string title;
    std::string author;
    std::string language;
    std::string uid;
    std::string coverImagePath;   // path inside ZIP
};

struct SpineItem {
    std::string id;
    std::string href;             // path inside ZIP
    bool linear;
};

struct TocEntry {
    std::string label;
    std::string href;
    int playOrder;
    std::vector<TocEntry> children;
};

struct BookModel {
    BookMetadata meta;
    std::vector<SpineItem> spine;
    std::vector<TocEntry> toc;
    std::string opfBaseDir;       // for resolving relative paths
};
```

Create `include/epub/OpfParser.h` + `src/epub/OpfParser.cpp`:
- Read `META-INF/container.xml` to locate the OPF file
- Parse OPF manifest, spine, and metadata
- Parse TOC: NCX for EPUB2, nav.xhtml for EPUB3 (detect automatically)
- Resolve all href paths relative to OPF location
- Extract cover image to `/fs/.paperread/epub_<hash>/cover.jpg`

**Deliverables:**
- `ZipReader` and `OpfParser` implementations
- Unit tests in `test/unit/test_opf_parser.cpp`
- Test fixtures in `test/fixtures/epubs/` — include a known EPUB2 and EPUB3 sample
- Commit: `[epub] Add ZIP reader and OPF parser`

**Acceptance Criteria:**
- Parses at least 5 sample EPUBs from Project Gutenberg (mix of EPUB2 and EPUB3) with correct metadata
- Cover image extraction works for all samples that have one
- TOC parsing produces correct nested structure
- Unit tests pass

---

### Task 2.2 — XHTML Chapter Parser `[parallel with 2.1]`

**Owner:** Firmware Engineer (Core)
**Depends on:** Phase 1

**Context:** Each chapter is an XHTML file inside the EPUB ZIP. Convert it to a flat list of layout blocks. Full CSS is not needed — structural tags only.

**Reference code:** `lib/Epub/RubbishHtmlParser/` in atomic14's repo for a simple approach; CrossPoint's `src/epub/html/` for a more sophisticated one.

**Instructions:**

Create `include/epub/BlockModel.h`:
```cpp
enum class BlockType { PARAGRAPH, HEADING, IMAGE, HORIZONTAL_RULE, BLANK };
enum class InlineStyle : uint8_t {
    NORMAL      = 0,
    BOLD        = 1 << 0,
    ITALIC      = 1 << 1,
    BOLD_ITALIC = BOLD | ITALIC,
};

struct InlineRun {
    std::string text;
    InlineStyle style;
};

struct Block {
    BlockType type;
    int headingLevel;              // 1–6 for HEADING
    std::vector<InlineRun> runs;
    std::string imagePath;         // ZIP-relative path for IMAGE
    int imageWidth, imageHeight;   // 0 if unknown
};

using BlockList = std::vector<Block>;
```

Create `include/epub/XhtmlParser.h` + `src/epub/XhtmlParser.cpp`:

Parse XHTML using TinyXML2. Tag mapping:
| Tags | Maps to |
|---|---|
| `p`, `div`, `section` | PARAGRAPH block |
| `h1`–`h6` | HEADING with matching level |
| `img` | IMAGE with src attribute |
| `hr` | HORIZONTAL_RULE |
| `b`, `strong` | Inline BOLD |
| `i`, `em` | Inline ITALIC |
| `br` | Split current inline run |
| `a` | Plain span (strip href) |
| Others | Strip tag, preserve text |

- Merge adjacent runs with identical style
- Drop empty blocks
- Be tolerant of malformed markup — never crash on bad input

**Deliverables:**
- `XhtmlParser` implementation
- Unit tests in `test/unit/test_xhtml_parser.cpp`
- XHTML fixtures in `test/fixtures/xhtml/` covering all tag types
- Commit: `[epub] Add XHTML chapter parser`

**Acceptance Criteria:**
- Correct `BlockList` output for all fixture files
- Handles nested inline formatting (`<b><i>text</i></b>`)
- Does not crash on malformed XHTML
- Unit tests pass

---

### Task 2.3 — Layout Engine `[sequential]`

**Owner:** Firmware Engineer (Core)
**Depends on:** 2.1, 2.2, Phase 3 Task 3.1 (font renderer — can stub initially)

**Context:** Take a `BlockList` plus font config, produce paginated `Page` objects ready for rendering. This is the most complex component — take care.

**Reference:** CrossPoint's layout engine is the best reference. Atomic14's word-wrap via dynamic programming is simpler but less capable.

**Instructions:**

Create `include/epub/LayoutModel.h`:
```cpp
struct LayoutWord {
    std::string text;
    InlineStyle style;
    int x, y;          // position within page (pixels)
    int width;         // measured pixel width
};

struct LayoutLine {
    std::vector<LayoutWord> words;
    int y;
    int height;
};

struct LayoutImage {
    std::string imagePath;
    int x, y, width, height;
};

struct LayoutElement {
    enum class Kind { LINE, IMAGE };
    Kind kind;
    LayoutLine line;
    LayoutImage image;
};

struct Page {
    std::vector<LayoutElement> elements;
    int chapterIndex;
    int blockStartIndex;  // first block on this page
    int blockEndIndex;    // one past last block
};

using PageList = std::vector<Page>;
```

Create `include/epub/LayoutEngine.h` + `src/epub/LayoutEngine.cpp`:
```cpp
class LayoutEngine {
public:
    struct Config {
        int pageWidth, pageHeight;
        int marginTop, marginBottom, marginLeft, marginRight;
        int lineSpacing;              // extra pixels between lines
        float fontSizeBody;           // in pt
        float fontSizeH1, fontSizeH2, fontSizeH3;
        bool justify;
    };

    LayoutEngine(IFontRenderer& fonts, Config config);
    PageList layout(const BlockList& blocks, int chapterIndex);
};
```

**Algorithm:**
1. For each block, measure content dimensions using the font renderer
2. For text blocks, word-wrap using dynamic programming (minimise raggedness)
3. For image blocks, scale to fit page width preserving aspect ratio
4. Pack elements onto pages, starting a new page when content exceeds page height
5. Record `blockStartIndex` and `blockEndIndex` per page for fast re-layout

**Performance targets:**
- Lay out a 100 KB chapter in < 2 seconds
- Cache results to `/fs/.paperread/epub_<hash>/sections/<n>.bin` with a binary serialisation format

**Deliverables:**
- `LayoutEngine` implementation
- Binary serialisation for `Page`/`PageList`
- Unit tests with a mock `IFontRenderer` returning fixed glyph widths
- Commit: `[epub] Add layout engine with page caching`

**Acceptance Criteria:**
- Produces sensible page breaks on fixture content
- No orphaned headings (heading always followed by at least one line of body)
- No widows/orphans at paragraph boundaries (nice-to-have, not blocker)
- Round-trips through cache serialisation correctly

---

## Phase 3 — Rendering & Fonts

**Goal:** Render layouts to the e-ink display.
**Duration:** 2–3 agent sessions

### Task 3.1 — FreeType Font Renderer `[sequential]`

**Owner:** Firmware Engineer (Core)
**Depends on:** Phase 1

**Context:** The juicecultus fork ships a prebuilt `lib_freetype` static library. Use it.

**Instructions:**

Create `include/fonts/IFontRenderer.h`:
```cpp
struct GlyphMetrics {
    int advance;        // horizontal advance in pixels
    int bearingX, bearingY;
    int width, height;  // bitmap dimensions
};

class IFontRenderer {
public:
    virtual ~IFontRenderer() = default;
    virtual bool loadFont(const std::string& path) = 0;
    virtual void setSize(float sizePt) = 0;
    virtual void setStyle(InlineStyle style) = 0;

    // Measure only (no rasterisation)
    virtual int measureText(const std::string& utf8) = 0;

    // Rasterise a glyph into an 8-bit grayscale bitmap (alpha)
    virtual bool rasteriseGlyph(uint32_t codepoint, std::vector<uint8_t>& bitmap, GlyphMetrics& metrics) = 0;

    virtual int lineHeight() = 0;
    virtual int ascent() = 0;
    virtual int descent() = 0;
};
```

Create `src/fonts/FreeTypeRenderer.cpp`:
- Wrap FreeType 2 API
- Implement a glyph cache (PSRAM-resident LRU, bounded size)
- Support regular/bold/italic/bold-italic via separate font files OR FreeType style emboldening/slanting
- UTF-8 decoding to codepoints

**Deliverables:**
- FreeType renderer implementation
- Glyph cache with configurable memory budget
- Unit test using a mock font or embedded test font
- Commit: `[fonts] Add FreeType renderer with glyph cache`

**Acceptance Criteria:**
- Renders Latin, accented, and a sample of Cyrillic glyphs correctly
- Glyph cache evicts oldest entries when memory limit reached
- Benchmark: measure a 1000-word paragraph in < 100 ms

---

### Task 3.2 — Page Renderer `[sequential]`

**Owner:** Firmware Engineer (Core)
**Depends on:** 3.1, 2.3

**Context:** Take a `Page` and produce a framebuffer ready to flush to the display.

**Instructions:**

Create `include/ui/PageRenderer.h` + `src/ui/PageRenderer.cpp`:
```cpp
class PageRenderer {
public:
    PageRenderer(IDisplay& display, IFontRenderer& fonts);

    // Render a page into the provided framebuffer
    void render(const Page& page, uint8_t* framebuffer, bool darkMode = false);

    // Render UI chrome (status bar, page indicator)
    void renderStatusBar(uint8_t* framebuffer, int batteryPercent, int currentPage, int totalPages);
};
```

- For text: rasterise each glyph and blit alpha into framebuffer at word position
- For images: decode JPEG/PNG (use ESP-IDF `jpeg_decoder` or tjpgd) and dither to 16-level grayscale
- Support dark mode by inverting the framebuffer before flush
- Implement simple ordered-dithering for image grayscale

**Deliverables:**
- `PageRenderer` implementation
- Image decoding for JPEG + PNG
- Commit: `[ui] Add page renderer`

**Acceptance Criteria:**
- Renders a test page correctly on device
- Text is crisp, properly spaced
- Images scale and dither reasonably
- Dark mode inverts correctly

---

## Phase 4 — Application Framework

**Goal:** Activity/screen system, input dispatch, settings persistence.
**Duration:** 2–3 agent sessions

### Task 4.1 — Settings Persistence `[parallel]`

**Owner:** Firmware Engineer (UX)
**Depends on:** Phase 1

**Instructions:**

Create `include/storage/Settings.h` + `src/storage/Settings.cpp`:
```cpp
struct Settings {
    // Typography
    int fontSize = 2;              // index into size table
    float lineSpacing = 1.2f;
    std::string fontFile;          // empty = use bundled

    // Display
    bool darkMode = false;
    Orientation rotation = Orientation::PORTRAIT;
    bool autoRotate = true;
    RefreshMode refreshQuality = RefreshMode::BALANCED_GL16;

    // Power
    int sleepTimeoutSec = 60;
    int autoPoweroffMin = 30;

    // Network
    std::string wifiSsid;
    std::string wifiPassword;
    std::string koreaderSyncUrl;
    std::string koreaderUsername;
    std::string koreaderPassword;
};

bool loadSettings(Settings& out);
bool saveSettings(const Settings& in);
```

Use ArduinoJson for serialisation. Store at `/fs/.paperread/settings.json`.

**Deliverables:**
- Settings load/save with JSON
- Unit tests for round-trip serialisation
- Commit: `[storage] Add settings persistence`

---

### Task 4.2 — Progress Tracking `[parallel]`

**Owner:** Firmware Engineer (UX)
**Depends on:** Phase 1

**Instructions:**

Create `include/storage/Progress.h` + `src/storage/Progress.cpp`:
```cpp
struct ReadingProgress {
    int chapterIndex;
    int pageInChapter;
    int furthestChapter;
    int furthestPage;
    time_t lastOpenedAt;
};

bool loadProgress(const std::string& bookCacheKey, ReadingProgress& out);
bool saveProgress(const std::string& bookCacheKey, const ReadingProgress& in);
```

Store as binary at `/fs/.paperread/epub_<hash>/progress.bin`.

**Deliverables:**
- Progress load/save
- Unit tests
- Commit: `[storage] Add reading progress persistence`

---

### Task 4.3 — Activity Framework `[sequential]`

**Owner:** Firmware Engineer (UX)
**Depends on:** 4.1, 4.2, 3.2

**Context:** A simple stack-based activity system inspired by Android.

**Instructions:**

Create `include/ui/Activity.h`:
```cpp
class Activity {
public:
    virtual ~Activity() = default;
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void onTouch(const TouchPoint& p) {}
    virtual void onSwipe(int dx, int dy) {}
    virtual void onButton() {}
    virtual void onTick() {}         // periodic update
    virtual bool needsRedraw() = 0;
    virtual void draw(uint8_t* framebuffer) = 0;
    virtual RefreshMode refreshMode() { return RefreshMode::BALANCED_GL16; }
};

class ActivityStack {
public:
    void push(std::unique_ptr<Activity> activity);
    void pop();
    void replace(std::unique_ptr<Activity> activity);
    Activity* top();
};
```

Create `include/ui/InputDispatcher.h`:
- Polls `ITouch` and detects: tap, long-press, swipe direction, pinch (optional)
- Debounces physical button
- Dispatches events to top of activity stack

Create `src/main/AppLoop.cpp` — the main FreeRTOS task:
1. Init board, load settings
2. Push initial activity (LibraryActivity)
3. Loop: read input, dispatch, check for redraw, render, sleep on inactivity

**Deliverables:**
- Activity framework + input dispatcher
- Main app loop
- Commit: `[ui] Add activity framework and main loop`

**Acceptance Criteria:**
- Pushing/popping activities works
- Input events dispatch to correct activity
- Tick rate reasonable (target 10 Hz for touch polling)

---

## Phase 5 — Core Activities

**Goal:** Library, reader, settings, sleep screens.
**Duration:** 3–4 agent sessions
**Parallelisation:** 5.1, 5.2, 5.3, 5.4 can run in parallel once framework is stable.

### Task 5.1 — Library Activity `[parallel]`

**Owner:** Firmware Engineer (UX)
**Depends on:** Phase 4

**Features:**
- Scan `/fs/Books/` recursively for `.epub` files
- Display scrollable grid of book covers (3-4 per row)
- Tap to open book (push ReaderActivity)
- Long-press for context menu: delete, metadata
- Subfolder navigation (breadcrumb at top)
- Show reading progress indicator on each cover

**Deliverables:** `src/ui/LibraryActivity.{h,cpp}`, commit `[ui] Add library activity`

---

### Task 5.2 — Reader Activity `[parallel]`

**Owner:** Firmware Engineer (UX)
**Depends on:** Phase 4, 2.3

**Features:**
- Open a book, parse, layout (with cache)
- Swipe/tap left/right to turn pages
- Swipe up from bottom: open in-reader menu (TOC, settings, bookmarks, go-to-page)
- Auto-save progress on every page turn
- Status bar: battery, page indicator, book title

**Deliverables:** `src/ui/ReaderActivity.{h,cpp}`, commit `[ui] Add reader activity`

---

### Task 5.3 — Settings Activity `[parallel]`

**Owner:** Firmware Engineer (UX)
**Depends on:** Phase 4

**Features:**
- Touch-friendly settings screens
- Font size, line spacing, margins, font file picker
- Dark mode toggle, rotation lock, refresh quality
- WiFi setup with on-screen keyboard
- KOReader sync config
- Sleep/power settings

**Deliverables:** `src/ui/SettingsActivity.{h,cpp}`, touch keyboard in `src/ui/Keyboard.{h,cpp}`, commit `[ui] Add settings activity`

---

### Task 5.4 — Sleep Screen `[parallel]`

**Owner:** Firmware Engineer (UX)
**Depends on:** Phase 4

**Features:**
- Show current book's cover with "Sleeping..." or timestamp overlay
- On wake, restore previous activity
- Optional hourly battery refresh (reference erbopubreader)

**Deliverables:** `src/ui/SleepActivity.{h,cpp}`, commit `[ui] Add sleep screen`

---

## Phase 6 — Networking

**Goal:** WiFi upload, OTA, KOReader sync.
**Duration:** 3–4 agent sessions

### Task 6.1 — WiFi Manager `[sequential]`

**Owner:** Firmware Engineer (Network)
**Depends on:** Phase 4

**Instructions:**
- `src/net/WifiManager.{h,cpp}` using ESP-IDF wifi APIs
- Connect using credentials from Settings
- Show QR code on screen containing WiFi info + device IP
- Handle reconnect, timeouts

**Deliverables:** Commit `[net] Add WiFi manager with QR display`

---

### Task 6.2 — HTTP Server for Book Upload `[sequential]`

**Owner:** Firmware Engineer (Network)
**Depends on:** 6.1

**Instructions:**
- Use `esp_http_server`
- Endpoints:
  - `GET /` — simple HTML UI with upload form and book list
  - `POST /upload` — multipart upload, stream to `/fs/Books/`
  - `GET /books` — JSON list of books
  - `DELETE /books/<name>` — remove a book
- Embed HTML/CSS/JS as C++ string constants (pre-minified by a build script)

**Deliverables:** Commit `[net] Add book upload HTTP server`

---

### Task 6.3 — OTA Firmware Updates `[sequential]`

**Owner:** Firmware Engineer (Network)
**Depends on:** 6.2

**Instructions:**
- Add `POST /ota` endpoint accepting `.bin` upload
- Use ESP-IDF OTA APIs (`esp_ota_begin`, `esp_ota_write`, `esp_ota_end`, `esp_ota_set_boot_partition`)
- Verify image, reboot on success
- Partition table: dual app slots for rollback (copy CrossPoint's `partitions.csv` as reference)

**Deliverables:** Commit `[net] Add OTA firmware update endpoint`

---

### Task 6.4 — KOReader Sync Client `[sequential]`

**Owner:** Firmware Engineer (Network)
**Depends on:** 6.1

**Context:** KOReader Sync is an HTTP-based protocol. CrossPoint has a working implementation with an important TLS OOM fix in the jpirnay fork.

**Instructions:**
- Port the CrossPoint sync client to PaperRead
- Apply the jpirnay TLS OOM fix
- Push progress on page turns (debounced to every 30s max)
- Pull progress on book open

**Deliverables:** Commit `[net] Add KOReader sync client`

---

## Phase 7 — Polish & Paper S3 Advantages

**Goal:** Features that exceed CrossPoint by using Paper S3's unique capabilities.
**Duration:** 3+ agent sessions
**Parallelisation:** All tasks independent.

### Task 7.1 — USB Disk Mode `[parallel]`

Reference: erbopubreader v1.0.2
- Expose SD card as USB mass storage when plugged in
- Eject mode to unmount before resume

### Task 7.2 — WiFi NTP + Clock Screen `[parallel]`

Reference: erbopubreader v1.0.12
- Sync time on WiFi connect
- Optional clock screen activity

### Task 7.3 — RTC Alarm Wake `[parallel]`

Reference: erbopubreader v1.0.13
- Settings to schedule wake-up
- Show alarm screen + play buzzer

### Task 7.4 — Bookmarks & Search `[parallel]`

Reference: erbopubreader v1.0.6
- Add bookmark from in-reader menu
- Full-text search across current book
- Go-to-page dialog

### Task 7.5 — OPDS Browser `[parallel, optional]`

- Connect to public OPDS catalogues
- Browse and download books directly to SD

---

## Phase 8 — Quality & Release

**Goal:** Production-ready firmware.
**Duration:** 2–3 agent sessions

### Task 8.1 — Integration Test Suite `[sequential]`

**Owner:** QA Engineer
**Depends on:** Phase 7

**Instructions:**
- Device-side test runner (`test/integration/`)
- Tests: EPUB load time, page turn latency, memory stability over 100 page turns, WiFi upload, OTA cycle, sleep/wake
- CI workflow: run unit tests on PR; manual integration runs for releases

**Deliverables:** Test suite, `.github/workflows/ci.yml`, commit `[test] Add integration test suite and CI`

---

### Task 8.2 — User Documentation `[parallel]`

**Owner:** Any agent
**Depends on:** feature-complete firmware

**Deliverables:**
- `docs/USER_GUIDE.md`
- `docs/FLASHING.md` (both web-flasher and esptool paths)
- `docs/DEVELOPMENT.md` (how to build and contribute)
- `README.md` pointing to each

---

### Task 8.3 — Web Flasher `[parallel]`

**Owner:** Any agent

- GitHub Pages site with ESP Web Tools
- Flashes latest firmware.bin via browser USB
- Reference CrossPoint's `xteink.dve.al` approach

**Deliverables:** `web-flasher/` directory, GitHub Pages config, commit `[release] Add web flasher`

---

### Task 8.4 — Release Workflow `[sequential]`

**Owner:** Architect

**Instructions:**
- `.github/workflows/release.yml` triggered on git tags
- Builds `firmware.bin`, `bootloader.bin`, `partitions.bin`
- Publishes GitHub Release with artefacts
- Updates web flasher with new version

**Deliverables:** Release workflow, commit `[release] Add automated release workflow`

---

## Progress Tracking

Use GitHub Project columns:
- **Backlog** — tasks not yet started
- **In Progress** — one agent actively working
- **Review** — PR open, awaiting review
- **Done** — merged to main

Each task becomes one GitHub Issue with a link back to the relevant section of this doc. Use labels: `phase-1`, `phase-2`, …, `parallel`, `sequential`, and role labels (`hal`, `core`, `ux`, `network`, `qa`).

---

## Escalation Rules

Agents should stop and escalate (rather than guess) when:
- A hardware pinout or register value is uncertain — verify against M5Stack docs first
- A dependency license is unclear (especially around M5Unified, epdiy forks)
- Memory budgets are being exceeded despite PSRAM usage
- A change would break the HAL contract or modify public interfaces
- Test coverage drops below 60% on changed files

Escalation = write findings to `docs/blockers/<task-id>.md` and open a GitHub Issue tagged `blocker`.

---

## Definition of Done (Project)

The project is complete when:
- All Phase 1–6 tasks are merged and passing CI
- At least 3 Phase 7 tasks are complete
- A user can flash the web flasher, load an EPUB, and read it without touching a terminal
- Documentation covers all user-facing features
- A v1.0.0 release is published with firmware artefacts

---

*PaperRead S3 Agent Build Plan — v1.0 — April 2026*
