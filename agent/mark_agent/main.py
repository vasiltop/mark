import json
from typing import Any

import uvicorn
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from sse_starlette.sse import EventSourceResponse

from mark_agent.config import settings
from mark_agent.graph import build_graph


class RunRequest(BaseModel):
  prompt: str
  source: str | None = None


app = FastAPI(title="Mark Agent")
app.add_middleware(
  CORSMiddleware,
  allow_origins=["http://localhost:5173"],
  allow_methods=["*"],
  allow_headers=["*"],
)


@app.get("/agent/health")
def health() -> dict[str, str]:
  return {"status": "ok", "model": settings.ollama_model}


@app.post("/agent/run")
async def run(request: RunRequest) -> EventSourceResponse:
  async def events():
    graph = build_graph()
    initial: dict[str, Any] = {
      "messages": [],
      "prompt": request.prompt,
      "outline": "",
      "source": request.source or "",
      "context": {},
      "compile_result": None,
      "iteration": 0,
      "steps": [],
      "skip_planner": bool(request.source),
    }
    if request.source:
      initial["outline"] = "Use and refine the provided source."
      initial["steps"] = ["writer: using editor source"]

    seen_steps = 0
    final_state: dict[str, Any] | None = None
    for update in graph.stream(initial):
      for _node, node_state in update.items():
        final_state = node_state
        steps = node_state.get("steps", [])
        while seen_steps < len(steps):
          yield {"event": "step", "data": json.dumps({"message": steps[seen_steps]})}
          seen_steps += 1
        if node_state.get("source"):
          yield {"event": "source", "data": json.dumps({"source": node_state["source"]})}

    if final_state:
      compile_result = final_state.get("compile_result")
      if compile_result:
        yield {"event": "compile", "data": json.dumps(compile_result)}
      status = "done" if compile_result and compile_result.get("status") == "done" else "error"
      yield {
        "event": "done",
        "data": json.dumps({
          "status": status,
          "source": final_state.get("source", ""),
          "outline": final_state.get("outline", ""),
        }),
      }

  return EventSourceResponse(events())


def main() -> None:
  uvicorn.run(
    "mark_agent.main:app",
    host="0.0.0.0",
    port=settings.agent_port,
    reload=False,
  )
