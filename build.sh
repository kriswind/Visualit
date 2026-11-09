#!/usr/bin/env bash
# Builds visualit and installs it for the current user:
#   <prefix>/bin/visualit, <prefix>/bin/vis (alias),
#   <prefix>/share/applications/visualit.desktop,
#   <prefix>/share/icons/hicolor/scalable/apps/visualit.svg
# The desktop entry is what Wayland compositors use for the window/taskbar icon.
#
# Usage: ./build.sh [--prefix DIR] [--debug] [--no-install]
#   --prefix DIR   install location (default: ~/.local)
#   --debug        Debug build instead of Release
#   --no-install   build only
set -euo pipefail

src_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
prefix="$HOME/.local"
build_type=Release
install=1

while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix) prefix="${2:?--prefix needs a directory}"; shift 2 ;;
        --prefix=*) prefix="${1#*=}"; shift ;;
        --debug) build_type=Debug; shift ;;
        --no-install) install=0; shift ;;
        -h|--help) sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "Unknown option: $1 (see --help)" >&2; exit 2 ;;
    esac
done

prefix="$(mkdir -p "$prefix" && cd "$prefix" && pwd)"
build_dir="$src_dir/build"

generator=()
if [[ ! -f "$build_dir/CMakeCache.txt" ]] && command -v ninja >/dev/null; then
    generator=(-G Ninja)
fi

# The prefix is baked into the desktop entry's Exec path, so it is set at configure time.
cmake -S "$src_dir" -B "$build_dir" "${generator[@]}" \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "$build_dir" --parallel

if [[ $install -eq 0 ]]; then
    echo "Built $build_dir/visualit"
    exit 0
fi

cmake --install "$build_dir"

# Refresh desktop caches so launchers and the compositor pick up the entry and icon.
icon_root="$prefix/share/icons/hicolor"
command -v gtk-update-icon-cache >/dev/null && gtk-update-icon-cache -q -f -t "$icon_root" 2>/dev/null || true
command -v update-desktop-database >/dev/null && update-desktop-database -q "$prefix/share/applications" || true
command -v kbuildsycoca6 >/dev/null && kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
# KDE apps (including the Plasma taskbar) cache failed icon lookups in-process; this signal
# makes every KIconLoader drop its cache, so a "visualit" lookup that missed before install retries.
command -v dbus-send >/dev/null && dbus-send --session --type=signal /KIconLoader \
    org.kde.KIconLoader.iconChanged int32:0 2>/dev/null || true

# Make sure the bin directory is on PATH for future shells.
bin_dir="$prefix/bin"
case ":$PATH:" in
    *":$bin_dir:"*)
        echo "Installed. Run: vis --help"
        ;;
    *)
        line="export PATH=\"$bin_dir:\$PATH\""
        case "$(basename "${SHELL:-}")" in
            zsh)  rc="$HOME/.zshrc" ;;
            bash) rc="$HOME/.bashrc" ;;
            fish) rc="" ;;
            *)    rc="$HOME/.profile" ;;
        esac
        if [[ -z "$rc" ]]; then
            fish -c "fish_add_path -U '$bin_dir'"
            echo "Added $bin_dir to fish's PATH."
        elif ! grep -qsF "$line" "$rc"; then
            printf '\n# Added by visualit build.sh\n%s\n' "$line" >> "$rc"
            echo "Added $bin_dir to PATH in $rc."
        fi
        echo "Installed. Open a new shell (or: source ${rc:-your fish config}), then run: vis --help"
        ;;
esac
