# Contributing to KeyloggerSniffer_PIO

Thank you for your interest in improving this project! Here's how to get started.

## How to Contribute

### Reporting Bugs

* Use the [Bug Report](../../issues/new?template=bug_report.md) issue template
* Include your board model, Arduino core version, and serial output

### Suggesting Features

* Use the [Feature Request](../../issues/new?template=feature_request.md) issue template
* Explain the use case and expected behavior

### Submitting Code

1. **Fork** the repository
2. Create a **feature branch** from `main`:
   ```
   git checkout -b feature/your-feature-name
   ```
3. Make your changes following the code style below
4. **Test** on real hardware (Raspberry Pi Pico)
5. **Commit** with a clear message:
   ```
   git commit -m "Add: brief description of what was added"
   ```
6. **Push** and open a **Pull Request**

## Code Style

* **Language**: C++ (Arduino-compatible)
* **Comments**: English, Doxygen-style (`@brief`, `@param`, `@return`) for public API
* **Indentation**: 4 spaces (no tabs)
* **Braces**: K&R style (opening brace on same line)
* **Naming**: `camelCase` for functions, `UPPER_CASE` for constants, `PascalCase` for files and structs
* **Headers**: `#pragma once` (no `#ifndef` guards)
* **Casts**: Use C++ casts (`static_cast<>`) instead of C-style casts

## Commit Message Convention

```
Add:      new feature or file
Fix:      bug fix
Docs:     documentation only
Refactor: code change that doesn't fix a bug or add a feature
i18n:     translation addition or correction
```

## What We're Looking For

* Additional keyboard layout tables
* Additional interface language translations
* AES-128 cipher backend for RP2350 hardware acceleration
* Offline decryption companion tool (Python)
* Improved auto-detection heuristics for keyboard layouts
* Documentation improvements and translations

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).
