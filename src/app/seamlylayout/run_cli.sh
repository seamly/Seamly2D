#!/usr/bin/env bash
# project: SeamlyLayout
# author: slspencer, copyright 2026
# MIT License: https://opensource.org/licenses/MIT
set -euo pipefail

# Build all workspace crates in release mode.
cargo build --release

# Run the CLI; forward any arguments to select subcommands (e.g., render, info).
cargo run --release -p cli -- "$@"

