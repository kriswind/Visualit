<p align="center">
  <img src="icons/visualit.svg" width="96" alt="Visualit icon">
</p>

# Visualit

A desktop dashboard for exploring the branches and commit history of a git repository, built with Qt 6 (C++ and QML).

Browse the commit graph, inspect any commit, and press **Enter** to open it on the repository's web host (GitHub, GitLab, Bitbucket, Azure DevOps, or any host with a `/commit/<sha>` page).

## Features

- **Commit graph** with coloured lanes for branches and merges, plus branch and tag labels on each commit.
- **Dashboard** with branch, commit, contributor and tag counts, time since the last commit, and a 26-week activity chart.
- **Branch list** with local and remote branches, newest first. Select one to see only its history.
- **Commit details** with the full message, author and committer, clickable parent commits, and changed files with lines added and removed.
- **Compact view** (Ctrl+M) that shows only the graph. Click a node to open the commit, or hover over it for a summary.
- **Search** by message, author, email, hash or branch name.
- Follows the system's light or dark setting.

## Requirements

- Qt 6.8 or newer (Gui, Quick, QuickControls2), plus the Qt SVG plugin for the icon
- CMake 3.16 or newer and a C++20 compiler
- `git` on `PATH`

On Arch Linux:

```sh
sudo pacman -S qt6-base qt6-declarative qt6-svg cmake ninja git
```

## Build and install

```sh
./build.sh
```

This builds a Release version and installs it for the current user under `~/.local`:

| File | Purpose |
| --- | --- |
| `~/.local/bin/visualit` | the application |
| `~/.local/bin/vis` | short alias |
| `~/.local/share/applications/visualit.desktop` | launcher entry; Wayland also uses it for the window icon |
| `~/.local/share/icons/hicolor/scalable/apps/visualit.svg` | icon |

If `~/.local/bin` is not on your `PATH`, the script adds it to your shell's startup file.

Options:

```sh
./build.sh --prefix DIR    # install somewhere other than ~/.local
./build.sh --debug         # Debug build
./build.sh --no-install    # build only, into ./build
```

To build with plain CMake instead:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix ~/.local
```

## Usage

```sh
vis                       # repository in the current directory
vis path/to/repo
vis . --web-url https://git.example.com/team/project
vis --help
```

The web URL for commit links comes from the `origin` remote, or the first remote if there is no `origin`. Use `--web-url` when it can't be worked out, or to point somewhere else.

### Keyboard

| Key | Action |
| --- | --- |
| ↑ ↓, PgUp/PgDn, Home/End | move through commits |
| Enter | open the selected commit in the browser |
| Ctrl+M | toggle compact view |
| Ctrl+F | search; Enter next match, Shift+Enter previous, Esc back to the list |
| Ctrl+O | open another repository |
| F5 / Ctrl+R | reload |

The app loads up to 5,000 commits. The commit count shows a `+` when it reaches that limit.
