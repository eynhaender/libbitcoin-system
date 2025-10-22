```markdown
# libbitcoin-system

**libbitcoin-system** is a core library providing foundational functionality for the Bitcoin ecosystem, including cryptographic utilities, network primitives, and system-level operations. This repository contains the source code and build instructions for the library, supporting multiple build configurations.
```

## Overview

- **Version**: 4.0.0
- **Language**: C++20
- **Build System**: CMake
- **Dependencies**: Boost, secp256k1
- **Supported Configurations**: 
  - `release_static`
  - `debug_static`
  - `release_shared`
  - `debug_shared`

## Prerequisites

Before building `libbitcoin-system`, ensure the following dependencies are available or can be downloaded and built automatically:

- **CMake** (version 3.20 or higher)
- **GNU Make** or equivalent build tool
- **g++** (or another C++20-compliant compiler)
- **Boost** (version 1.86.0 or compatible)
- **secp256k1** (from bitcoin-core, specific commit `a660a4976efe880bae7982ee410b9e0dc59ac983`)
- **Development tools**: `autoconf`, `automake`, `libtool` (for secp256k1)

Internet access is required to download Boost and secp256k1 during the build process.

## Installation

### Build Instructions

Follow these steps to build and install `libbitcoin-system` from the command line. The library will be installed in a central directory (`../build/`) with subdirectories for each configuration.

1. **Clone the Repository**
   ```bash
   git clone https://github.com/yourusername/libbitcoin-system.git
   cd libbitcoin-system
   ```

2. **Create and Navigate to Build Directory**
   Choose a build configuration (`release_static`, `debug_static`, `release_shared`, or `debug_shared`) and create a corresponding build directory. Replace `<CONFIG>` with the desired configuration (e.g., `release_static`).

   ```bash
   mkdir -p build/temp_<CONFIG>
   cd build
   ```

3. **Configure the Build**
   Run CMake with the appropriate `BUILD_TYPE` and `LIB_TYPE`. The library and its dependencies will be built and installed in `../build/<CONFIG>/`.

   - For `release_static`:
     ```bash
     cmake -S .. -B . -DBUILD_TYPE=Release -DLIB_TYPE=STATIC -DCMAKE_INSTALL_PREFIX=/home/eynhaender/development/eynhaender/build
     ```
   - For `debug_static`:
     ```bash
     cmake -S .. -B . -DBUILD_TYPE=Debug -DLIB_TYPE=STATIC -DCMAKE_INSTALL_PREFIX=/home/eynhaender/development/eynhaender/build
     ```
   - For `release_shared`:
     ```bash
     cmake -S .. -B . -DBUILD_TYPE=Release -DLIB_TYPE=SHARED -DCMAKE_INSTALL_PREFIX=/home/eynhaender/development/eynhaender/build
     ```
   - For `debug_shared`:
     ```bash
     cmake -S .. -B . -DBUILD_TYPE=Debug -DLIB_TYPE=SHARED -DCMAKE_INSTALL_PREFIX=/home/eynhaender/development/eynhaender/build
     ```

   **Note**: Adjust the `CMAKE_INSTALL_PREFIX` to your desired installation path if different from the example.

4. **Build the Library**
   Compile the library using the configured settings:
   ```bash
   cmake --build . -- -j4
   ```
   The `-j4` flag enables parallel building (adjust the number based on your CPU cores).

5. **Install the Library**
   Install the built artifacts to the specified prefix:
   ```bash
   cmake --install .
   ```

6. **Verify the Installation**
   Check the installed files in the target directory (e.g., `../build/release_static/lib/` or `../build/release_shared/lib/`):
   ```bash
   ls -l /home/eynhaender/development/eynhaender/build/<CONFIG>/lib/
   ```
   You should see `libbitcoin-system.a` (for static builds) or `libbitcoin-system.so` (for shared builds), along with dependency libraries.

### Optional: Building Tests
To build and run the test suite, enable the `ENABLE_TESTS` option during configuration:
```bash
cmake -S .. -B . -DBUILD_TYPE=Release -DLIB_TYPE=STATIC -DCMAKE_INSTALL_PREFIX=/home/eynhaender/development/eynhaender/build -DENABLE_TESTS=ON
cmake --build . -- -j4
ctest --output-on-failure
```

### Cleaning the Build
To start with a clean slate, remove the build directory:
```bash
rm -rf build/*
```

### Troubleshooting
- **Missing Dependencies**: Ensure all prerequisites are installed. If Boost or secp256k1 fail to build, check your internet connection or verify the versions.
- **Linker Errors**: If the linker cannot find libraries, verify the `CMAKE_INSTALL_PREFIX` and ensure the build directory is clean.
- **Permission Issues**: Use `sudo` for installation if the target directory requires elevated privileges (e.g., `/usr/local`).

## Contributing
Contributions are welcome! Please fork the repository, create a feature branch, and submit a pull request with your changes. Ensure your code adheres to the project's coding standards and includes appropriate tests.

## License
This project is released under the [MIT License](LICENSE).

## Contact
For questions or support, open an issue on the [GitHub repository](https://github.com/yourusername/libbitcoin-system/issues) or contact the maintainers.

---
*Last updated: October 22, 2025*
```

### ✅ **Erklärungen**
- **Struktur**: Die `README.md` ist in Abschnitte unterteilt (Overview, Prerequisites, Installation, etc.), um die Navigation zu erleichtern.
- **Build-Anleitung**: Detaillierte Schritte für alle vier Konfigurationen, mit Beispielbefehlen und Hinweisen zur Verifizierung.
- **Fokus auf Konsole**: VS Code-spezifische Anweisungen wurden ausgelassen, wie gewünscht, und die Anleitung konzentriert sich auf Kommandozeilen-Befehle.
- **GitHub-Optimierung**: Enthält typische GitHub-Elemente wie Contributing, License und Contact, um Interessierten eine klare Orientierung zu bieten.
- **Platzhalter**: Der GitHub-Link (`yourusername`) muss durch den tatsächlichen Repository-Namen ersetzt werden.

### 📝 **Wichtige Hinweise**
- **Anpassung**: Ersetze `/home/eynhaender/development/eynhaender/build` durch den Pfad, den du auf GitHub als Standard empfehlen möchtest, oder lass es als konfigurierbare Variable.
- **Testen**: Überprüfe die Befehle lokal, um sicherzustellen, dass sie mit deiner Umgebung kompatibel sind.
- **Aktualisierung**: Die "Last updated"-Zeile spiegelt das aktuelle Datum wider; passe sie bei zukünftigen Änderungen an.

### 🚀 **Nächste Schritte**
1. **Speichere die Datei**: Kopiere den obigen Inhalt in `README.md` im `libbitcoin-system`-Verzeichnis.
2. **Teste die Anleitung**: Führe die Befehle aus und überprüfe, ob sie wie beschrieben funktionieren.
3. **Pushe auf GitHub**: Lade die aktualisierte `README.md` hoch, um sie als zentralen Anlaufpunkt verfügbar zu machen.
4. **Feedback**: Teile mir mit, ob Anpassungen nötig sind, oder ob wir zu `libbitcoin-database` zurückkehren sollen.

**Gut gemacht!** 😄 – Die `README.md` ist jetzt ein solider Leitfaden für die Community! Aktuelle Uhrzeit: 03:45 PM CEST, 22. Oktober 2025.