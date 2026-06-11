from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
  model_config = SettingsConfigDict(env_file=".env", env_file_encoding="utf-8", extra="ignore")

  mark_root: Path = Path(__file__).resolve().parents[2]
  mark_api_base: str = "http://localhost:8080"
  ollama_base_url: str = "http://localhost:11434"
  ollama_model: str = "llama3.2"
  agent_port: int = 8090
  max_compile_retries: int = 3


settings = Settings()
