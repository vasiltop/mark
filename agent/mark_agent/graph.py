import re
from typing import Any, Literal

from langchain_core.messages import HumanMessage, SystemMessage
from langchain_ollama import ChatOllama
from langgraph.graph import END, StateGraph

from mark_agent.config import settings
from mark_agent.prompts import PLANNER_SYSTEM, WRITER_SYSTEM, language_context
from mark_agent.state import AgentState
from mark_agent.tools.compile import compile_mark


def make_llm() -> ChatOllama:
  return ChatOllama(
    model=settings.ollama_model,
    base_url=settings.ollama_url,
    temperature=0.2,
  )


def strip_fences(text: str) -> str:
  fenced = re.search(r"```(?:mark)?\s*\n?(.*?)```", text, re.DOTALL | re.IGNORECASE)
  if fenced:
    return fenced.group(1).strip()
  return text.strip()


def planner_node(state: AgentState) -> dict[str, Any]:
  llm = make_llm()
  response = llm.invoke([
    SystemMessage(content=PLANNER_SYSTEM),
    HumanMessage(content=state["prompt"]),
  ])
  outline = str(response.content).strip()
  steps = [*state.get("steps", []), "planner: outline ready"]
  return {"outline": outline, "steps": steps}


def writer_node(state: AgentState) -> dict[str, Any]:
  llm = make_llm()
  compile_result = state.get("compile_result")
  error_hint = ""
  if compile_result and compile_result.get("status") == "error":
    line = compile_result.get("errorLine")
    prefix = f"line {line}: " if line else ""
    error_hint = (
      f"\n\nFix this compile error:\n{prefix}{compile_result.get('error', 'unknown error')}"
    )

  user_content = (
    f"User request:\n{state['prompt']}\n\nOutline:\n{state.get('outline', '')}{error_hint}"
  )
  if state.get("source") and compile_result and compile_result.get("status") == "error":
    user_content += f"\n\nPrevious source:\n{state['source']}"

  response = llm.invoke([
    SystemMessage(content=WRITER_SYSTEM.format(language_context=language_context())),
    HumanMessage(content=user_content),
  ])
  source = strip_fences(str(response.content))
  iteration = state.get("iteration", 0)
  if compile_result and compile_result.get("status") == "error":
    iteration += 1
  steps = [*state.get("steps", []), "writer: draft updated"]
  return {"source": source, "iteration": iteration, "steps": steps, "compile_result": None}


def reviewer_node(state: AgentState) -> dict[str, Any]:
  context = state.get("context") or {}
  if not context.get("title") and state.get("prompt"):
    context = {**context, "title": state["prompt"][:80]}

  result = compile_mark(state["source"], context)
  compile_result = result.to_dict()
  attempt = state.get("iteration", 0) + 1
  label = "reviewer: compile ok" if result.status == "done" else f"reviewer: compile failed (attempt {attempt})"
  steps = [*state.get("steps", []), label]
  return {"compile_result": compile_result, "context": context, "steps": steps}


def route_after_review(state: AgentState) -> Literal["writer", "__end__"]:
  compile_result = state.get("compile_result") or {}
  if compile_result.get("status") == "done":
    return END
  if state.get("iteration", 0) >= settings.max_compile_retries:
    return END
  return "writer"


def route_start(state: AgentState) -> Literal["planner", "writer"]:
  if state.get("skip_planner"):
    return "writer"
  return "planner"


def build_graph():
  graph = StateGraph(AgentState)
  graph.add_node("planner", planner_node)
  graph.add_node("writer", writer_node)
  graph.add_node("reviewer", reviewer_node)
  graph.set_conditional_entry_point(
    route_start,
    {"planner": "planner", "writer": "writer"},
  )
  graph.add_edge("planner", "writer")
  graph.add_edge("writer", "reviewer")
  graph.add_conditional_edges("reviewer", route_after_review, {"writer": "writer", END: END})
  return graph.compile()


def run_agent(prompt: str, source: str | None = None) -> AgentState:
  app = build_graph()
  initial: AgentState = {
    "messages": [],
    "prompt": prompt,
    "outline": "",
    "source": source or "",
    "context": {},
    "compile_result": None,
    "iteration": 0,
    "steps": [],
    "skip_planner": bool(source),
  }
  if source:
    initial["outline"] = "Use and refine the provided source."
    initial["steps"] = ["writer: using editor source"]
  return app.invoke(initial)
