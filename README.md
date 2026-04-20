# PaperRead S3 📚

**A modern, feature-rich EPUB reader firmware for the M5Stack Paper S3**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP--IDF-orange.svg)](https://platformio.org/)

---

## About

PaperRead S3 is a production-quality EPUB reader designed specifically for the M5Stack Paper S3 (ESP32-S3 with 960×540 e-ink display). It combines the best features from existing open-source readers with a clean architecture and modern capabilities.

### Key Features

**Core Reading:**
- EPUB 2 & 3 support with full metadata parsing
- Advanced layout engine with pagination
- FreeType font rendering with glyph caching
- Dark mode with customizable display settings
- Touch navigation with gesture support

**Library Management:**
- Book library with cover grid view
- Nested folder support
- Reading progress tracking
- Bookmark management

**Connectivity:**
- WiFi book upload via web interface
- Over-the-air (OTA) firmware updates
- KOReader sync integration
- Optional OPDS catalog support

**Hardware Features:**
- USB mass storage mode
- RTC alarms and clock screen
- Battery monitoring
- IMU-based auto-rotation
- Multiple e-ink refresh modes

---

## Hardware Requirements

- **M5Stack Paper S3**
  - ESP32-S3R8 (8 MB PSRAM, 16 MB flash)
  - 960×540 e-ink display
  - GT911 capacitive touch
  - BMI270 IMU
  - BM8563 RTC
  - SD card slot

---

## Quick Start

### 1. Flash Firmware (Web-Based - Easiest)

Visit our web flasher: **[Coming Soon]**

### 2. Build from Source

#### Prerequisites
- [PlatformIO](https://platformio.org/)
- Git
- USB cable for your M5Stack Paper S3

#### Build & Flash
```bash
# Clone repository with submodules
git clone --recursive https://github.com/clipstick/paperread-s3.git
cd paperread-s3

# Build for Paper S3
pio run -e paper_s3_idf

# Flash to device
pio run -e paper_s3_idf -t upload

# Monitor serial output
pio device monitor
```

---

## Documentation

- **[Build Plan](docs/AGENT_BUILD_PLAN.md)** - Complete development roadmap
- **[User Guide](docs/USER_GUIDE_PAPER_S3.md)** - How to use the reader
- **[Development Guide](docs/DEVELOPMENT.md)** - Contributing and building *(coming soon)*
- **[Flashing Guide](docs/FLASHING.md)** - Installation instructions *(coming soon)*

---

## Project Status

🚧 **Phase 0: Repository Bootstrap** - Complete ✓

This project is currently in initial development. See [AGENT_BUILD_PLAN.md](docs/AGENT_BUILD_PLAN.md) for the complete implementation roadmap.

### Roadmap

- [x] Phase 0: Repository setup and baseline verification
- [ ] Phase 1: Hardware abstraction layer (HAL)
- [ ] Phase 2: EPUB parsing engine
- [ ] Phase 3: Rendering & fonts
- [ ] Phase 4: Application framework
- [ ] Phase 5: Core UI activities
- [ ] Phase 6: Networking features
- [ ] Phase 7: Polish & advanced features
- [ ] Phase 8: Testing & release

---

## Architecture

PaperRead S3 uses a clean, layered architecture:

```
┌─────────────────────────────────────┐
│   Activities (Library, Reader, UI)  │
├─────────────────────────────────────┤
│   EPUB Engine (Parse, Layout)       │
├─────────────────────────────────────┤
│   Rendering (Fonts, Display)        │
├─────────────────────────────────────┤
│   Hardware Abstraction Layer (HAL)  │
├─────────────────────────────────────┤
│   Hardware (ESP32-S3, epdiy, GT911) │
└─────────────────────────────────────┘
```

All business logic is hardware-agnostic, making the code portable and testable.

---

## Building on Giants

PaperRead S3 builds upon excellent prior work:

- **[juicecultus/diy-esp32s3-epub-reader](https://github.com/juicecultus/diy-esp32s3-epub-reader)** - Base Paper S3 implementation
- **[crosspoint-reader](https://github.com/crosspoint-reader/crosspoint-reader)** - Feature and UX reference
- **[erbopubreader](https://erbosoft.org)** - Dark mode, USB, NTP implementations
- **[atomic14/diy-esp32-epub-reader](https://github.com/atomic14/diy-esp32-epub-reader)** - Parsing reference
- **[vroland/epdiy](https://github.com/vroland/epdiy)** - E-ink panel driver

---

## Development

### Code Conventions
- C++20 standard
- Clean separation: business logic never calls hardware directly
- Comprehensive unit testing (target 70%+ coverage)
- Commit format: `[component] Description`

### Memory Management
- All allocations >4KB use PSRAM
- Framebuffers always in PSRAM
- RAII wrappers for safe memory handling

---

## Contributing

Contributions are welcome! This project uses an agent-based development workflow:

1. Check [AGENT_BUILD_PLAN.md](docs/AGENT_BUILD_PLAN.md) for current tasks
2. Pick a task or propose a new feature
3. Follow the code conventions
4. Submit a PR with tests
5. Ensure CI passes

---

## License

MIT License - see [LICENSE](LICENSE) for details.

---

## Acknowledgments

Thanks to the ESP32 and e-ink communities, especially:
- Martin Bertin for epdiy rotation support
- The atomic14 team for pioneering ESP32 EPUB readers
- The CrossPoint team for UX inspiration
- Erik Botta for advanced reader features

---

**Made with ❤️ for the open-source e-reader community**
