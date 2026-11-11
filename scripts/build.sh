#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Reuse an active ESP-IDF environment, or honor an explicit toolchain setup.
if [[ -n ${IDF_TOOLCHAIN_BIN:-} ]]; then
    export PATH="$IDF_TOOLCHAIN_BIN:$PATH"
fi
if [[ -n ${IDF_PYTHON_ENV_PATH:-} ]]; then
    export PATH="$IDF_PYTHON_ENV_PATH/bin:$PATH"
fi
if [[ -n ${IDF_PATH:-} && -n ${IDF_PYTHON_ENV_PATH:-} ]]; then
    idf=(python "$IDF_PATH/tools/idf.py")
elif command -v idf.py >/dev/null; then
    idf=(idf.py)
else
    echo 'Activate the ESP-IDF 5.3.1 environment before building.' >&2
    exit 1
fi
if [[ $# -eq 0 ]]; then set -- build; fi
exec "${idf[@]}" "$@"
