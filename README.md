# Mark

Typst-like markup language that compiles to HTML.

## Quick start

```bash
just dev
```

## Common commands

```bash
just build              # C++ compiler and worker
just compile            # compile examples/hello.mark to stdout
just compile-out        # compile to out.html
just ci                 # run CI checks locally
just ollama-up          # start ollama and pull default model
just agent-install      # uv sync agent deps
just agent              # LangGraph agent on :8090
just stop               # stop worker and backend
just down               # stop everything including Kafka
just status             # show what's running
just help               # list all recipes
```

## Agent

Multi-agent document authoring with LangGraph and Ollama. The agent plans, writes Mark source, and compiles via the existing Spring API until the document succeeds or retries are exhausted.

Configure in `agent/.env` (see `agent/.env.example`):

- `OLLAMA_MODEL` — default `llama3.2`
- `MARK_API_BASE` — default `http://localhost:8080`

## Docs

See [docs/language-spec.md](docs/language-spec.md).
