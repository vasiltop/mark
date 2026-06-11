import json
import time
from dataclasses import dataclass
from typing import Any

import httpx

from mark_agent.config import settings


@dataclass
class CompileResult:
  status: str
  html: str | None = None
  css: str | None = None
  error: str | None = None
  error_line: int | None = None
  error_col: int | None = None

  def to_dict(self) -> dict[str, Any]:
    return {
      "status": self.status,
      "html": self.html,
      "css": self.css,
      "error": self.error,
      "errorLine": self.error_line,
      "errorCol": self.error_col,
    }


def compile_mark(source: str, context: dict[str, Any] | None = None) -> CompileResult:
  payload: dict[str, Any] = {"source": source}
  if context:
    payload["context"] = context

  with httpx.Client(base_url=settings.mark_api_base, timeout=30.0) as client:
    create = client.post("/api/jobs", json=payload)
    create.raise_for_status()
    job = create.json()
    job_id = job["jobId"]

    for _ in range(120):
      poll = client.get(f"/api/jobs/{job_id}")
      poll.raise_for_status()
      result = poll.json()
      status = result.get("status")
      if status == "done":
        return CompileResult(
          status="done",
          html=result.get("html"),
          css=result.get("css"),
        )
      if status == "error":
        return CompileResult(
          status="error",
          error=result.get("error"),
          error_line=result.get("errorLine"),
          error_col=result.get("errorCol"),
        )
      time.sleep(0.5)

  return CompileResult(status="error", error="compile timed out")
