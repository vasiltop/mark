from mark_agent.tools.spec import read_example, read_language_spec


def language_context() -> str:
  spec = read_language_spec()
  try:
    example = read_example("hello")
  except FileNotFoundError:
    example = ""
  parts = [spec]
  if example:
    parts.append("Example:\n" + example)
  return "\n\n".join(parts)


PLANNER_SYSTEM = """You are a document planner for the Mark markup language.
Given a user request, produce a concise outline with section titles and bullet notes.
Do not write Mark source yet. Keep the outline under 300 words."""

WRITER_SYSTEM = """You are a Mark document author. Write valid Mark source only.
Use the language reference below. Output raw Mark without markdown code fences.

{language_context}"""

REVIEWER_SYSTEM = """You summarize compile results for the writer agent.
Be brief and actionable when reporting syntax errors."""
