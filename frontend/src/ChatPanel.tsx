import { useRef, useState } from 'react'

type AgentDone = {
  status: string
  source: string
  outline: string
}

type ChatPanelProps = {
  getSource: () => string
  onApplySource: (source: string) => void
}

function parseSseBlock(block: string): { event: string; data: string } | null {
  const lines = block.split('\n')
  let event = 'message'
  let data = ''
  for (const line of lines) {
    const trimmed = line.replace(/\r$/, '')
    if (trimmed.startsWith('event:')) event = trimmed.slice(6).trim()
    else if (trimmed.startsWith('data:')) data += trimmed.slice(5).trim()
  }
  if (!data) return null
  return { event, data }
}

export function ChatPanel({ getSource, onApplySource }: ChatPanelProps) {
  const [prompt, setPrompt] = useState('')
  const [steps, setSteps] = useState<string[]>([])
  const [status, setStatus] = useState<'idle' | 'running' | 'done' | 'error'>('idle')
  const [draftSource, setDraftSource] = useState('')
  const [error, setError] = useState<string | null>(null)
  const abortRef = useRef<AbortController | null>(null)

  function handleEvent(event: string, data: string) {
    if (event === 'step') {
      const payload = JSON.parse(data) as { message: string }
      setSteps((prev) => [...prev, payload.message])
      return
    }
    if (event === 'source') {
      const payload = JSON.parse(data) as { source: string }
      if (payload.source) setDraftSource(payload.source)
      return
    }
    if (event === 'done') {
      const payload = JSON.parse(data) as AgentDone
      if (payload.source) setDraftSource(payload.source)
      setStatus(payload.status === 'done' ? 'done' : 'error')
      if (payload.status !== 'done') {
        setError('agent finished without a successful compile')
      }
    }
  }

  function consumeBuffer(buffer: string): string {
    const blocks = buffer.split('\n\n')
    const rest = blocks.pop() ?? ''
    for (const block of blocks) {
      const parsed = parseSseBlock(block)
      if (parsed) handleEvent(parsed.event, parsed.data)
    }
    return rest
  }

  async function runAgent() {
    const currentSource = getSource()
    if (!prompt.trim()) return
    abortRef.current?.abort()
    const controller = new AbortController()
    abortRef.current = controller

    setStatus('running')
    setSteps([])
    setDraftSource('')
    setError(null)

    try {
      const response = await fetch('/agent/run', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          Accept: 'text/event-stream',
        },
        body: JSON.stringify({
          prompt: prompt.trim(),
          source: currentSource.trim() || undefined,
        }),
        signal: controller.signal,
      })

      if (!response.ok || !response.body) {
        throw new Error('agent request failed')
      }

      const reader = response.body.getReader()
      const decoder = new TextDecoder()
      let buffer = ''

      while (true) {
        const { done, value } = await reader.read()
        if (value) {
          buffer += decoder.decode(value, { stream: true })
          buffer = consumeBuffer(buffer)
        }
        if (done) break
      }

      buffer += decoder.decode()
      if (buffer.trim()) {
        const parsed = parseSseBlock(buffer)
        if (parsed) handleEvent(parsed.event, parsed.data)
      }
    } catch (err) {
      if (controller.signal.aborted) return
      setStatus('error')
      setError(err instanceof Error ? err.message : 'unknown error')
    }
  }

  return (
    <aside className="chat-pane">
      <div className="pane-title">Agent · Ollama</div>
      <div className="chat-body">
        <textarea
          className="chat-input"
          value={prompt}
          onChange={(e) => setPrompt(e.target.value)}
          placeholder="Describe a Mark document to generate…"
          rows={4}
          disabled={status === 'running'}
        />
        <div className="chat-actions">
          <button type="button" onClick={() => void runAgent()} disabled={status === 'running' || !prompt.trim()}>
            Generate
          </button>
          <button
            type="button"
            className="secondary"
            onClick={() => draftSource && onApplySource(draftSource)}
            disabled={!draftSource}
          >
            Apply to editor
          </button>
          <span className="status">{status}</span>
        </div>
        {error && <p className="error chat-error">{error}</p>}
        {steps.length > 0 && (
          <ol className="chat-steps">
            {steps.map((step, i) => (
              <li key={i}>{step}</li>
            ))}
          </ol>
        )}
      </div>
    </aside>
  )
}
