#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

mkdir -p databases
rm -f databases/normal_database.db databases/page_database.db

touch databases/normal_database.db databases/page_database.db
mkdir -p "$REPO_ROOT/executables"

gcc -std=c11 -Wall -Wextra -Iheaders \
    layers/logger.c \
    layers/disk_manager.c \
    layers/slotted_page.c \
    layers/heap_file.c \
    layers/table_api.c \
    layers/page.c \
    benchmark/benchmark.c \
    -o "$REPO_ROOT/executables/benchmark_exec"

"$REPO_ROOT/executables/benchmark_exec"
