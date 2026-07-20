#!/usr/bin/env bash
# project: SeamlyLayout
# author: slspencer, copyright 2026
# MIT License: https://opensource.org/licenses/MIT
set -euo pipefail

if [ "$#" -ne 2 ]; then
  echo "usage: $0 <input.svg> <output.png>" 1>&2
  exit 1
fi

input="$1"
output="$2"

# Delegate to the CLI wrapper with a fixed scale of 1.0.
"$(dirname "$0")/run_cli.sh" render --input "$input" --output "$output" --scale 1.0

