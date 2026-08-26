#!/usr/bin/env bash
# Collect libraries, demo binary and pkg-config file for CI artifact upload.
set -euo pipefail

platform="${1:?usage: stage-artifacts.sh linux|windows|macos}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
dest="$root/ci-artifacts/$platform"

mkdir -p "$dest/lib" "$dest/bin" "$dest/include"

copy_if_exists()
{
    local file="$1"
    local target_dir="$2"
    if [[ -f "$file" ]]; then
        cp "$file" "$target_dir/"
        return 0
    fi
    echo "Missing expected file: $file" >&2
    return 1
}

case "$platform" in
    linux)
        copy_if_exists "$root/build/libOpenGlassBox.a" "$dest/lib"
        copy_if_exists "$root/build/libOpenGlassBox.so" "$dest/lib"
        copy_if_exists "$root/build/OpenGlassBox-demo" "$dest/bin"
        chmod +x "$dest/bin/OpenGlassBox-demo" "$dest/lib/"*.so*
        ;;
    windows)
        copy_if_exists "$root/build/libOpenGlassBox.a" "$dest/lib"
        copy_if_exists "$root/build/libOpenGlassBox.dll" "$dest/lib"
        copy_if_exists "$root/build/OpenGlassBox-demo.exe" "$dest/bin"
        ;;
    macos)
        copy_if_exists "$root/build/libOpenGlassBox.a" "$dest/lib"
        copy_if_exists "$root/build/libOpenGlassBox.dylib" "$dest/lib"
        if [[ -d "$root/build/OpenGlassBox-demo.app" ]]; then
            cp -R "$root/build/OpenGlassBox-demo.app" "$dest/bin/"
        elif [[ -f "$root/build/OpenGlassBox-demo" ]]; then
            copy_if_exists "$root/build/OpenGlassBox-demo" "$dest/bin"
            chmod +x "$dest/bin/OpenGlassBox-demo"
        else
            echo "Missing demo binary or .app bundle under build/" >&2
            exit 1
        fi
        ;;
    *)
        echo "Unknown platform: $platform" >&2
        exit 1
        ;;
esac

copy_if_exists "$root/build/OpenGlassBox.pc" "$dest/lib"
cp -R "$root/include/OpenGlassBox" "$dest/include/"
[[ -f "$root/LICENSE" ]] && cp "$root/LICENSE" "$dest/"
grep '^PROJECT_VERSION' "$root/Makefile.common" | awk '{print $3}' > "$dest/VERSION"

echo "Staged $platform artifacts:"
find "$dest" -type f | sort
