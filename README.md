# Mark

A Typst-like markup language that compiles to HTML.

## Usage

```bash
./run.sh
./bin/mark examples/hello.mark > out.html
```

## Features

- Markup: headings, emphasis, lists, cross-refs
- Code: `#link`, `#image`, `#table`, `#set` rules
- Unity-build C++ compiler in `compiler/`
- Kafka worker stub in `worker/`
