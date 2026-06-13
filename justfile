# Mark local development

set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

root := justfile_directory()
dev_dir := root + "/.dev"
jar := root + "/backend/target/backend-0.0.1-SNAPSHOT.jar"

default:
    @just --list

# Run the same checks as .github/workflows/ci.yml
ci:
    #!/usr/bin/env bash
    set -euo pipefail
    cd "{{root}}"
    mkdir -p bin
    g++ -Icompiler compiler/mark.cpp -Wall -Wextra -std=c++23 -g -o bin/mark
    g++ -Icompiler worker/worker.cpp -Wall -Wextra -std=c++23 -g -lrdkafka -o bin/mark-worker
    ./bin/mark examples/hello.mark | grep -q mark-doc
    ./bin/mark examples/academic.mark | grep -q mark-math
    if command -v mvn >/dev/null 2>&1; then
      mvn_bin=mvn
    elif [[ -n "${MAVEN:-}" && -x "${MAVEN}" ]]; then
      mvn_bin="${MAVEN}"
    else
      version=3.9.9
      mvn_home="{{dev_dir}}/apache-maven-${version}"
      if [[ ! -x "${mvn_home}/bin/mvn" ]]; then
        echo "mvn not found — downloading maven ${version} to .dev/"
        mkdir -p "{{dev_dir}}"
        curl -fsSL "https://archive.apache.org/dist/maven/maven-3/${version}/binaries/apache-maven-${version}-bin.tar.gz" \
          | tar xz -C "{{dev_dir}}"
      fi
      mvn_bin="${mvn_home}/bin/mvn"
    fi
    "${mvn_bin}" -f backend/pom.xml -B package -DskipTests
    test -f backend/target/backend-0.0.1-SNAPSHOT.jar
    npm ci --prefix frontend
    npm run build --prefix frontend
    npm run lint --prefix frontend

# Build binaries, install deps, and pull the default Ollama model
setup:
    #!/usr/bin/env bash
    set -euo pipefail
    cd "{{root}}"
    set -a
    [[ -f .env ]] && source .env
    set +a
    export PATH="$HOME/.local/bin:$PATH"
    mkdir -p bin "{{dev_dir}}/logs"

    if [[ ! -f .env && -f .env.example ]]; then
      cp .env.example .env
      echo "created .env from .env.example"
    fi

    : "${HOST:=localhost}"
    : "${BACKEND_PORT:=8080}"
    : "${AGENT_PORT:=8090}"
    : "${FRONTEND_PORT:=5173}"
    : "${KAFKA_PORT:=9092}"
    : "${OLLAMA_PORT:=11434}"
    : "${BACKEND_URL:=http://${HOST}:${BACKEND_PORT}}"
    : "${AGENT_URL:=http://${HOST}:${AGENT_PORT}}"
    : "${FRONTEND_URL:=http://${HOST}:${FRONTEND_PORT}}"
    : "${OLLAMA_URL:=http://${HOST}:${OLLAMA_PORT}}"
    : "${KAFKA_BOOTSTRAP:=${HOST}:${KAFKA_PORT}}"
    : "${OLLAMA_MODEL:=llama3.2}"

    echo "building C++ compiler and worker..."
    g++ -Icompiler compiler/mark.cpp -Wall -Wextra -std=c++23 -g -o bin/mark
    g++ -Icompiler worker/worker.cpp -Wall -Wextra -std=c++23 -g -lrdkafka -o bin/mark-worker

    echo "building backend..."
    if command -v mvn >/dev/null 2>&1; then
      mvn_bin=mvn
    elif [[ -n "${MAVEN:-}" && -x "${MAVEN}" ]]; then
      mvn_bin="${MAVEN}"
    else
      version=3.9.9
      mvn_home="{{dev_dir}}/apache-maven-${version}"
      if [[ ! -x "${mvn_home}/bin/mvn" ]]; then
        echo "mvn not found — downloading maven ${version} to .dev/"
        curl -fsSL "https://archive.apache.org/dist/maven/maven-3/${version}/binaries/apache-maven-${version}-bin.tar.gz" \
          | tar xz -C "{{dev_dir}}"
      fi
      mvn_bin="${mvn_home}/bin/mvn"
    fi
    "${mvn_bin}" -f backend/pom.xml -q package -DskipTests

    echo "installing frontend deps..."
    npm install --prefix frontend

    echo "installing agent deps..."
    uv sync --directory agent

    if command -v ollama >/dev/null 2>&1; then
      if ! curl -sf "${OLLAMA_URL}/api/tags" >/dev/null 2>&1; then
        echo "starting ollama..."
        nohup ollama serve >"{{dev_dir}}/logs/ollama.log" 2>&1 &
        echo $! >"{{dev_dir}}/ollama.pid"
        for _ in $(seq 1 30); do
          curl -sf "${OLLAMA_URL}/api/tags" >/dev/null 2>&1 && break
          sleep 1
        done
      fi
      if ! ollama list | grep -q "${OLLAMA_MODEL}"; then
        echo "pulling ${OLLAMA_MODEL}..."
        ollama pull "${OLLAMA_MODEL}"
      fi
    else
      echo "warning: ollama not installed — agent needs it (https://ollama.com)"
    fi

    echo "setup complete"

# Start Kafka, worker, backend, agent, and the Vite dev server
dev:
    #!/usr/bin/env bash
    set -euo pipefail
    cd "{{root}}"
    set -a
    [[ -f .env ]] && source .env
    set +a
    export PATH="$HOME/.local/bin:$PATH"
    mkdir -p "{{dev_dir}}/logs"

    : "${HOST:=localhost}"
    : "${BACKEND_PORT:=8080}"
    : "${AGENT_PORT:=8090}"
    : "${FRONTEND_PORT:=5173}"
    : "${KAFKA_PORT:=9092}"
    : "${OLLAMA_PORT:=11434}"
    : "${BACKEND_URL:=http://${HOST}:${BACKEND_PORT}}"
    : "${AGENT_URL:=http://${HOST}:${AGENT_PORT}}"
    : "${FRONTEND_URL:=http://${HOST}:${FRONTEND_PORT}}"
    : "${OLLAMA_URL:=http://${HOST}:${OLLAMA_PORT}}"
    : "${KAFKA_BOOTSTRAP:=${HOST}:${KAFKA_PORT}}"

    if [[ ! -x bin/mark-worker || ! -f "{{jar}}" ]]; then
      echo "missing build artifacts — run 'just setup' first"
      exit 1
    fi

    start_bg() {
      local name="$1" pattern="$2" workdir="$3"
      shift 3
      if pgrep -f "$pattern" >/dev/null 2>&1; then
        echo "$name already running"
        return
      fi
      echo "starting $name..."
      (cd "$workdir" && nohup "$@" >"{{dev_dir}}/logs/$name.log" 2>&1 & echo $! >"{{dev_dir}}/$name.pid")
    }

    wait_http() {
      local name="$1" url="$2"
      for _ in $(seq 1 60); do
        if curl -s -o /dev/null "$url" 2>/dev/null; then
          echo "$name ready"
          return
        fi
        sleep 1
      done
      echo "error: $name did not become ready — check {{dev_dir}}/logs/$name.log"
      exit 1
    }

    wait_tcp() {
      local name="$1" host="$2" port="$3"
      for _ in $(seq 1 60); do
        if (echo >/dev/tcp/"$host"/"$port") 2>/dev/null; then
          echo "$name ready"
          return
        fi
        sleep 1
      done
      echo "error: $name did not become ready — check {{dev_dir}}/logs/$name.log"
      exit 1
    }

    echo "starting kafka..."
    docker compose up -d zookeeper kafka
    for _ in $(seq 1 30); do
      docker compose exec -T kafka kafka-broker-api-versions --bootstrap-server "${KAFKA_BOOTSTRAP}" >/dev/null 2>&1 && break
      sleep 1
    done

    if command -v ollama >/dev/null 2>&1 && ! curl -sf "${OLLAMA_URL}/api/tags" >/dev/null 2>&1; then
      echo "starting ollama..."
      nohup ollama serve >"{{dev_dir}}/logs/ollama.log" 2>&1 &
      echo $! >"{{dev_dir}}/ollama.pid"
      wait_http ollama "${OLLAMA_URL}/api/tags"
    fi

    start_bg worker 'bin/mark-worker' "{{root}}" ./bin/mark-worker
    start_bg backend 'backend-0.0.1-SNAPSHOT.jar' "{{root}}" env BACKEND_PORT="${BACKEND_PORT}" FRONTEND_URL="${FRONTEND_URL}" KAFKA_BOOTSTRAP="${KAFKA_BOOTSTRAP}" java -jar "{{jar}}"
    wait_tcp backend "${HOST}" "${BACKEND_PORT}"

    start_bg agent 'mark-agent' "{{root}}/agent" env BACKEND_URL="${BACKEND_URL}" AGENT_URL="${AGENT_URL}" FRONTEND_URL="${FRONTEND_URL}" OLLAMA_URL="${OLLAMA_URL}" AGENT_PORT="${AGENT_PORT}" uv run mark-agent
    wait_http agent "${AGENT_URL}/agent/health"

    echo ""
    echo "Mark dev stack"
    echo "  editor   ${FRONTEND_URL}"
    echo "  backend  ${BACKEND_URL}/api/jobs"
    echo "  agent    ${AGENT_URL}/agent/health"
    echo "  ollama   ${OLLAMA_URL}"
    echo ""
    echo "Ctrl+C stops the frontend; run 'just stop' to stop everything else"
    echo ""

    cd frontend && npm run dev

# Stop worker, backend, agent, frontend, kafka, and ollama if we started it
stop:
    #!/usr/bin/env bash
    set -euo pipefail
    cd "{{root}}"
    set -a
    [[ -f .env ]] && source .env
    set +a

    stop_pid() {
      local name="$1" file="{{dev_dir}}/$1.pid"
      if [[ -f "$file" ]]; then
        local pid
        pid=$(<"$file")
        if kill -0 "$pid" 2>/dev/null; then
          kill "$pid" 2>/dev/null || true
          echo "stopped $name"
        fi
        rm -f "$file"
      fi
    }

    pkill -f 'bin/mark-worker' 2>/dev/null || true
    pkill -f 'backend-0.0.1-SNAPSHOT.jar' 2>/dev/null || true
    pkill -f 'mark-agent' 2>/dev/null || true
    pkill -f 'vite.*frontend' 2>/dev/null || true

    for name in worker backend agent ollama; do
      stop_pid "$name"
    done

    docker compose down 2>/dev/null || true
    echo "stopped all services"
