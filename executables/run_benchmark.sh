#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

mkdir -p databases
rm -f databases/normal_database.db databases/page_database.db

touch databases/normal_database.db databases/page_database.db

gcc -std=c11 -Wall -Wextra -Iheaders \
    layers/logger.c \
    layers/disk_manager.c \
    layers/slotted_page.c \
    layers/heap_file.c \
    layers/table_api.c \
    layers/page.c \
    benchmark/benchmark.c \
    -o "$SCRIPT_DIR/benchmark_exec"

"$SCRIPT_DIR/benchmark_exec"
