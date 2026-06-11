import { useMemo, useState } from 'react'
import './App.css'

type CompileJob = {
  jobId: string
  status: string
  html?: string | null
  css?: string | null
  error?: string | null
}

const defaultSource = `#set text(font-family: "Georgia, serif", font-size: 12pt)

= Hello Mark

This is a *strong* and _emphasized_ paragraph.

#link("https://example.com")[Example link]
`

async function sleep(ms: number) {
  return new Promise((resolve) => setTimeout(resolve, ms))
}

function App() {
  const [source, setSource] = useState(defaultSource)
  const [status, setStatus] = useState('idle')
  const [error, setError] = useState<string | null>(null)
  const [preview, setPreview] = useState('')

  const lineCount = useMemo(() => source.split('\n').length, [source])

  async function compile() {
    setStatus('submitting')
    setError(null)

    try {
      const create = await fetch('/api/jobs', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ source }),
      })

      if (!create.ok) throw new Error('failed to create job')
      const job: CompileJob = await create.json()

      setStatus('waiting')
      for (let i = 0; i < 60; i++) {
        await sleep(500)
        const res = await fetch(`/api/jobs/${job.jobId}`)
        if (!res.ok) throw new Error('failed to poll job')
        const current: CompileJob = await res.json()
        if (current.status === 'done') {
          setPreview(current.html ?? '')
          setStatus('done')
          return
        }
        if (current.status === 'error') {
          throw new Error(current.error ?? 'compile failed')
        }
      }
      throw new Error('timed out waiting for compile result')
    } catch (err) {
      setStatus('error')
      setError(err instanceof Error ? err.message : 'unknown error')
    }
  }

  return (
    <div className="app">
      <header>
        <h1>Mark</h1>
        <button type="button" onClick={compile} disabled={status === 'submitting' || status === 'waiting'}>
          Compile
        </button>
        <span className="status">{status}</span>
      </header>
      {error && <p className="error">{error}</p>}
      <div className="panes">
        <section className="editor-pane">
          <div className="pane-title">Source · {lineCount} lines</div>
          <textarea
            value={source}
            onChange={(e) => setSource(e.target.value)}
            spellCheck={false}
          />
        </section>
        <section className="preview-pane">
          <div className="pane-title">Preview</div>
          <iframe title="preview" srcDoc={preview} sandbox="allow-same-origin" />
        </section>
      </div>
    </div>
  )
}

export default App
