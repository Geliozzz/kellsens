#!/usr/bin/env bash
set -euo pipefail

BOARD="nucleo_l433rc_p/stm32l433xx"
MODEL="${MODEL:-ollama_chat/qwen2.5-coder:14b}"

export OLLAMA_API_BASE="${OLLAMA_API_BASE:-http://127.0.0.1:11434}"
source .venv/bin/activate
aider \
  src/main.c \
  prj.conf \
  app.overlay \
  CMakeLists.txt \
  --model "${MODEL}" \
  --test-cmd "west flash" \
  --auto-test


# --test-cmd "west build -b ${BOARD} -p always app" \

