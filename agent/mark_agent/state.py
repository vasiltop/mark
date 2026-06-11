from typing import Annotated, Any, TypedDict

from langgraph.graph.message import add_messages


class AgentState(TypedDict):
  messages: Annotated[list, add_messages]
  prompt: str
  outline: str
  source: str
  context: dict[str, Any]
  compile_result: dict[str, Any] | None
  iteration: int
  steps: list[str]
  skip_planner: bool
