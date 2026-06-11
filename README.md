# Mark

Typst-like markup language that compiles to HTML.

## Quick start

```bash
./run.sh
./bin/mark examples/hello.mark > out.html
```

## Full stack

```bash
docker-compose up -d zookeeper kafka
./bin/mark-worker &
java -jar backend/target/backend-0.0.1-SNAPSHOT.jar
cd frontend && npm install && npm run dev
```

Open http://localhost:5173 — edit `.mark` source, click Compile, preview HTML.

## Docs

See [docs/language-spec.md](docs/language-spec.md).
