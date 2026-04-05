#!/usr/bin/env bash
#
# package.sh — Build and package Mondfall: Letztes Gefecht for distribution
#
# Usage:
#   ./scripts/package.sh              # macOS app bundle (default)
#   ./scripts/package.sh macos        # macOS .app bundle
#   ./scripts/package.sh linux        # Linux tarball (requires linux toolchain)
#   ./scripts/package.sh windows      # Windows zip (requires mingw cross-compiler)
#   ./scripts/package.sh all          # All platforms
#
# Output goes to dist/

set -euo pipefail
cd "$(dirname "$0")/.."

VERSION="${VERSION:-1.0.0}"
APP_NAME="Mondfall-LetzesGefecht"
BUNDLE_ID="com.mondfall.game"
DIST_DIR="dist"
BUILD_DIR="build"

# ── Helpers ──────────────────────────────────────────────────────────

log()  { printf '\033[1;32m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[1;33m==> %s\033[0m\n' "$*"; }
die()  { printf '\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

clean_build() {
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR" "$DIST_DIR"
}

# Copies resources and sounds into a target directory
copy_assets() {
    local dest="$1"
    mkdir -p "$dest/resources" "$dest/sounds/soviet_death_sounds" "$dest/sounds/american_death_sounds"
    cp resources/crt.fs resources/hud.fs resources/intro.txt "$dest/resources/"
    cp sounds/soviet_death_sounds/*.mp3 "$dest/sounds/soviet_death_sounds/" 2>/dev/null || true
    cp sounds/american_death_sounds/*.mp3 "$dest/sounds/american_death_sounds/" 2>/dev/null || true
    cp sounds/march.wav "$dest/sounds/" 2>/dev/null || true
}

# ── macOS .app bundle ────────────────────────────────────────────────

package_macos() {
    log "Building macOS binary..."
    make clean
    make mondfall

    local app="$BUILD_DIR/${APP_NAME}.app"
    local contents="$app/Contents"
    local macos="$contents/MacOS"
    local res="$contents/Resources"

    log "Creating app bundle..."
    mkdir -p "$macos" "$res"

    # Binary
    cp mondfall "$macos/${APP_NAME}"

    # Assets alongside binary (raylib loads relative to executable)
    copy_assets "$macos"

    # Info.plist
    cat > "$contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>${APP_NAME}</string>
    <key>CFBundleDisplayName</key>
    <string>${APP_NAME} — Nazis on the Moon</string>
    <key>CFBundleIdentifier</key>
    <string>${BUNDLE_ID}</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundleExecutable</key>
    <string>${APP_NAME}</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>12.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.action-games</string>
</dict>
</plist>
PLIST

    # Package as zip for distribution
    local out="$DIST_DIR/${APP_NAME}-${VERSION}-macos.zip"
    log "Packaging $out..."
    (cd "$BUILD_DIR" && zip -r -y "../$out" "${APP_NAME}.app")

    log "macOS package: $out"
}

# ── Linux tarball ────────────────────────────────────────────────────

package_linux() {
    log "Building Linux binary..."

    local cc="${LINUX_CC:-x86_64-linux-gnu-gcc}"
    local raylib_linux="${RAYLIB_LINUX:-/usr/local/lib}"

    if ! command -v "$cc" &>/dev/null; then
        die "Linux cross-compiler '$cc' not found. Set LINUX_CC= or install a cross toolchain."
    fi

    clean_build

    local src
    src=$(grep '^SRC' Makefile | sed 's/^SRC = //')
    local all_src="$src vendor/flecs/flecs.c"

    # shellcheck disable=SC2086
    $cc -std=c99 -O2 -Ivendor/flecs -Isrc \
        $all_src \
        -L"$raylib_linux" -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
        -o "$BUILD_DIR/mondfall"

    local staging="$BUILD_DIR/${APP_NAME}-${VERSION}-linux"
    mkdir -p "$staging"
    cp "$BUILD_DIR/mondfall" "$staging/"
    copy_assets "$staging"

    local out="$DIST_DIR/${APP_NAME}-${VERSION}-linux.tar.gz"
    log "Packaging $out..."
    tar -czf "$out" -C "$BUILD_DIR" "$(basename "$staging")"

    log "Linux package: $out"
}

# ── Windows zip ──────────────────────────────────────────────────────

package_windows() {
    log "Building Windows binary..."

    local cc="${WINDOWS_CC:-x86_64-w64-mingw32-gcc}"
    local raylib_win="${RAYLIB_WINDOWS:-/usr/local/x86_64-w64-mingw32/lib}"

    if ! command -v "$cc" &>/dev/null; then
        die "MinGW cross-compiler '$cc' not found. Set WINDOWS_CC= or install mingw-w64."
    fi

    clean_build

    local src
    src=$(grep '^SRC' Makefile | sed 's/^SRC = //')
    local all_src="$src vendor/flecs/flecs.c"

    # shellcheck disable=SC2086
    $cc -std=c99 -O2 -Ivendor/flecs -Isrc \
        $all_src \
        -L"$raylib_win" -lraylib -lopengl32 -lgdi32 -lwinmm -lkernel32 -lshell32 -luser32 \
        -o "$BUILD_DIR/mondfall.exe" -mwindows

    local staging="$BUILD_DIR/${APP_NAME}-${VERSION}-windows"
    mkdir -p "$staging"
    cp "$BUILD_DIR/mondfall.exe" "$staging/"
    copy_assets "$staging"

    local out="$DIST_DIR/${APP_NAME}-${VERSION}-windows.zip"
    log "Packaging $out..."
    (cd "$BUILD_DIR" && zip -r "../$out" "$(basename "$staging")")

    log "Windows package: $out"
}

# ── Main ─────────────────────────────────────────────────────────────

platform="${1:-macos}"

case "$platform" in
    macos)   clean_build; package_macos   ;;
    linux)   clean_build; package_linux   ;;
    windows) clean_build; package_windows ;;
    all)
        clean_build
        package_macos
        package_linux
        package_windows
        ;;
    *)
        die "Unknown platform: $platform (use macos, linux, windows, or all)"
        ;;
esac

log "Done. Output in $DIST_DIR/"
