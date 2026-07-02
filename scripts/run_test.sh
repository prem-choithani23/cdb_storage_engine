#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

mkdir -p "$REPO_ROOT/executables"
rm -f database.db

gcc -std=c11 -Wall -Wextra -Iheaders \
    layers/logger.c \
    layers/disk_manager.c \
    layers/slotted_page.c \
    layers/heap_file.c \
    layers/table_api.c \
    layers/page.c \
    test.c \
    -o "$REPO_ROOT/executables/test_exec"

"$REPO_ROOT/executables/test_exec"
