import { useMemo, useRef, useState } from 'react'
import './App.css'

type CompileJob = {
  jobId: string
  status: string
  html?: string | null
  css?: string | null
  error?: string | null
  errorLine?: number | null
  errorCol?: number | null
}

const defaultSource = `#set text(font-family: "Georgia, serif", font-size: 12pt)
#set heading(numbering: "1.")

= Hello Mark

This is a *strong* and _emphasized_ paragraph with inline math $E = mc^2$.

#link("https://example.com")[Example link]
`

function App() {
  const [source, setSource] = useState(defaultSource)
  const [status, setStatus] = useState('idle')
  const [error, setError] = useState<string | null>(null)
  const [errorLine, setErrorLine] = useState<number | null>(null)
  const [preview, setPreview] = useState('')
  const textareaRef = useRef<HTMLTextAreaElement>(null)

  const lines = useMemo(() => source.split('\n'), [source])

  async function waitForJob(jobId: string): Promise<CompileJob> {
    return new Promise((resolve, reject) => {
      const source = new EventSource(`/api/jobs/${jobId}/events`)
      source.addEventListener('status', (event) => {
        const job: CompileJob = JSON.parse((event as MessageEvent).data)
        if (job.status === 'done') {
          source.close()
          resolve(job)
        } else if (job.status === 'error') {
          source.close()
          resolve(job)
        }
      })
      source.onerror = () => {
        source.close()
        reject(new Error('lost connection to compile stream'))
      }
    })
  }

  async function compile() {
    setStatus('submitting')
    setError(null)
    setErrorLine(null)

    try {
      const create = await fetch('/api/jobs', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ source }),
      })

      if (!create.ok) throw new Error('failed to create job')
      const job: CompileJob = await create.json()

      setStatus('waiting')
      const result = await waitForJob(job.jobId)
      if (result.status === 'done') {
        setPreview(result.html ?? '')
        setStatus('done')
        return
      }
      setError(result.error ?? 'compile failed')
      setErrorLine(result.errorLine ?? null)
      setStatus('error')
      if (result.errorLine && textareaRef.current) {
        const lineHeight = 1.5 * 15.2
        textareaRef.current.scrollTop = Math.max(0, (result.errorLine - 2) * lineHeight)
      }
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
      {error && (
        <p className="error">
          {errorLine ? `line ${errorLine}: ` : ''}
          {error}
        </p>
      )}
      <div className="panes">
        <section className="editor-pane">
          <div className="pane-title">Source · {lines.length} lines</div>
          <div className="editor-shell">
            <div className="gutter" aria-hidden="true">
              {lines.map((_, i) => (
                <div key={i} className={errorLine === i + 1 ? 'gutter-line error-line' : 'gutter-line'}>
                  {i + 1}
                </div>
              ))}
            </div>
            <textarea
              ref={textareaRef}
              value={source}
              onChange={(e) => setSource(e.target.value)}
              spellCheck={false}
              className={errorLine ? 'has-error' : ''}
            />
          </div>
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
