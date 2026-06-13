# Mark

Typst-like markup language that compiles to HTML.

## Quick start

```bash
just setup   # first time: build everything and pull deps
just dev     # start the full stack
just stop    # stop everything
```

Kafka runs in Docker; the worker, backend, frontend, and agent run on the host.

Copy [`.env.example`](.env.example) to `.env` at the repo root (or run `just setup`, which creates `.env` if missing). `just dev` loads it automatically.

Key variables:

- `BACKEND_URL`, `AGENT_URL`, `FRONTEND_URL`, `OLLAMA_URL` — service URLs
- `BACKEND_PORT`, `AGENT_PORT`, `FRONTEND_PORT`, `KAFKA_PORT`, `OLLAMA_PORT` — ports
- `KAFKA_BOOTSTRAP`, `KAFKA_JOBS_TOPIC`, `KAFKA_RESULTS_TOPIC` — Kafka
- `OLLAMA_MODEL` — default `llama3.2`

## Agent

Multi-agent document authoring with LangGraph and Ollama. The agent plans, writes Mark source, and compiles via the existing Spring API until the document succeeds or retries are exhausted.

## Docs

See [docs/language-spec.md](docs/language-spec.md).
