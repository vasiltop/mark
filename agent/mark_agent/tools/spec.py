from pathlib import Path

from mark_agent.config import settings


def read_language_spec() -> str:
  path = settings.mark_root / "docs" / "language-spec.md"
  return path.read_text(encoding="utf-8")


def read_example(name: str) -> str:
  for subdir in ("examples", "stdlib"):
    path = settings.mark_root / subdir / f"{name}.mark"
    if path.is_file():
      return path.read_text(encoding="utf-8")
  raise FileNotFoundError(f"example not found: {name}")
