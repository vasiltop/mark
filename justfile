# Mark dev — run `just` to start the full stack

set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

root := justfile_directory()

default:
    @just dev

# List recipes
help:
    @just --list

# Build C++ compiler CLI and Kafka worker
build:
    cd "{{root}}" && ./run.sh

# Build Spring Boot backend JAR
build-backend:
    cd "{{root}}" && backend/build.sh

# Install frontend deps if missing
frontend-install:
    #!/usr/bin/env bash
    cd "{{root}}/frontend"
    [[ -d node_modules ]] || npm install

# Start Zookeeper + Kafka
kafka-up:
    cd "{{root}}" && docker compose up -d zookeeper kafka

# Stop docker compose services
kafka-down:
    cd "{{root}}" && docker compose down

# Compile a .mark example to stdout
compile example="hello":
    cd "{{root}}" && ./bin/mark "examples/{{example}}.mark"

# Compile a .mark example to a file
compile-out example="hello" out="out.html":
    cd "{{root}}" && ./bin/mark "examples/{{example}}.mark" > "{{out}}"

# Build everything (C++, backend, frontend production bundle)
build-all: build build-backend
    cd "{{root}}/frontend" && npm install && npm run build

# Run the same checks as GitHub Actions CI
ci: build
    cd "{{root}}" && ./bin/mark examples/hello.mark >/dev/null
    cd "{{root}}" && ./bin/mark examples/academic.mark >/dev/null
    mvn -f backend/pom.xml -B package -DskipTests
    cd "{{root}}/frontend" && npm ci && npm run build && npm run lint

# Start Kafka, worker, backend, and the Vite dev server
dev:
    #!/usr/bin/env bash
    set -euo pipefail
    cd "{{root}}"

    just build
    just kafka-up
    echo "waiting for kafka..."
    sleep 4

    if [[ ! -f backend/target/backend-0.0.1-SNAPSHOT.jar ]]; then
      just build-backend
    fi
    just frontend-install

    if ! pgrep -f 'bin/mark-worker' >/dev/null 2>&1; then
      echo "starting mark-worker..."
      ./bin/mark-worker &
    else
      echo "mark-worker already running"
    fi

    if ! pgrep -f 'backend-0.0.1-SNAPSHOT.jar' >/dev/null 2>&1; then
      echo "starting backend..."
      java -jar backend/target/backend-0.0.1-SNAPSHOT.jar &
      sleep 2
    else
      echo "backend already running"
    fi

    echo ""
    echo "Mark dev stack"
    echo "  editor   http://localhost:5173"
    echo "  backend  http://localhost:8080/api/jobs"
    echo ""
    echo "Ctrl+C stops the frontend; run 'just stop' to stop worker and backend"
    echo ""

    cd frontend && npm run dev

# Start only infrastructure + worker + backend (no frontend)
dev-api: build kafka-up
    #!/usr/bin/env bash
    set -euo pipefail
    cd "{{root}}"
    sleep 4
    if [[ ! -f backend/target/backend-0.0.1-SNAPSHOT.jar ]]; then
      just build-backend
    fi
    if ! pgrep -f 'bin/mark-worker' >/dev/null 2>&1; then
      ./bin/mark-worker &
    fi
    if ! pgrep -f 'backend-0.0.1-SNAPSHOT.jar' >/dev/null 2>&1; then
      java -jar backend/target/backend-0.0.1-SNAPSHOT.jar &
    fi
    echo "API stack running on http://localhost:8080"

# Run worker in foreground
worker:
    cd "{{root}}" && ./bin/mark-worker

# Run backend in foreground
backend:
    cd "{{root}}" && java -jar backend/target/backend-0.0.1-SNAPSHOT.jar

# Run Vite dev server
frontend:
    cd "{{root}}/frontend" && npm run dev

# Stop local worker and backend
stop:
    #!/usr/bin/env bash
    pkill -f 'bin/mark-worker' 2>/dev/null || true
    pkill -f 'backend-0.0.1-SNAPSHOT.jar' 2>/dev/null || true
    echo "stopped worker and backend"

# Stop worker, backend, and kafka
down: stop kafka-down

# Show running services
status:
    #!/usr/bin/env bash
    cd "{{root}}"
    echo "kafka:"
    docker compose ps kafka 2>/dev/null | tail -n +2 || echo "  not running"
    echo "worker:"
    pgrep -af 'bin/mark-worker' || echo "  not running"
    echo "backend:"
    pgrep -af 'backend-0.0.1-SNAPSHOT.jar' || echo "  not running"
