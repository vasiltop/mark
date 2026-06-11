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
just stop               # stop worker and backend
just down               # stop everything including Kafka
just status             # show what's running
just help               # list all recipes
```

## Docs

See [docs/language-spec.md](docs/language-spec.md).
