# Mark

Typst-like markup language that compiles to HTML.

## Quick start

```bash
just dev
```

In another terminal, start the agent (requires [Ollama](https://ollama.com) with `llama3.2` or your chosen model):

```bash
ollama pull llama3.2
just agent-install
just agent
```

## Common commands

```bash
just build              # C++ compiler and worker
just compile            # compile examples/hello.mark to stdout
just compile-out        # compile to out.html
just ci                 # run CI checks locally
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

Demo prompt in the editor chat panel: *Write an academic Mark doc about Kafka with a table, inline math, and a link.*

## Docs

See [docs/language-spec.md](docs/language-spec.md).
