#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")" && pwd)"
cd "$root"

./run.sh

if ! docker compose ps kafka 2>/dev/null | grep -q running; then
  echo "starting kafka..."
  docker compose up -d zookeeper kafka
  sleep 5
fi

if ! pgrep -f bin/mark-worker >/dev/null; then
  echo "starting mark-worker..."
  ./bin/mark-worker &
fi

if ! pgrep -f backend-0.0.1-SNAPSHOT.jar >/dev/null; then
  if [[ ! -f backend/target/backend-0.0.1-SNAPSHOT.jar ]]; then
    backend/build.sh
  fi
  echo "starting backend..."
  java -jar backend/target/backend-0.0.1-SNAPSHOT.jar &
fi

echo "Mark dev stack running."
echo "  compiler: ./bin/mark examples/hello.mark"
echo "  frontend: cd frontend && npm run dev"
