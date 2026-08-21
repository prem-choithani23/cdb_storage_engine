#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

mkdir -p databases
rm -f databases/database.db
touch databases/database.db

mkdir -p "$REPO_ROOT/executables"

gcc -std=c11 -Wall -Wextra -Iheaders \
    src/logging/logger.c \
    src/disk/disk_manager.c \
    src/buffer/page_table.c \
    src/buffer/buffer_pool.c \
    src/storage/slotted_page.c \
    src/storage/heap_file.c \
    src/storage/table_api.c \
    src/storage/page.c \
    main.c \
    -o "$REPO_ROOT/executables/main_exec"

"$REPO_ROOT/executables/main_exec"