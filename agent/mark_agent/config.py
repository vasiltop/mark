from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict

_root = Path(__file__).resolve().parents[2]


class Settings(BaseSettings):
  model_config = SettingsConfigDict(
    env_file=str(_root / ".env"),
    env_file_encoding="utf-8",
    extra="ignore",
  )

  mark_root: Path = _root
  backend_url: str = "http://localhost:8080"
  ollama_url: str = "http://localhost:11434"
  frontend_url: str = "http://localhost:5173"
  ollama_model: str = "llama3.2"
  agent_port: int = 8090
  max_compile_retries: int = 3


settings = Settings()
