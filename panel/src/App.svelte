<script>
  const wsUrl = import.meta.env.VITE_FOVEA_WS_URL ?? 'ws://127.0.0.1:8765';

  const cameraModes = [
    { id: 'forward', label: 'Forward', available: true },
    { id: 'reverse', label: 'Reverse', available: false },
    { id: 'surround', label: '360°', available: false },
  ];

  let connected = $state(false);
  let scene = $state(null);
  let metrics = $state(null);
  let grok = $state({ text: '', error: '' });
  let camera = $state({ mode: 'forward', label: 'Forward Camera' });
  let lastFrameAt = $state(0);
  let fps = $state(0);

  const socket = new WebSocket(wsUrl);

  socket.addEventListener('open', () => {
    connected = true;
  });

  socket.addEventListener('close', () => {
    connected = false;
    fps = 0;
  });

  socket.addEventListener('message', (event) => {
    const payload = JSON.parse(event.data);
    if (payload.type !== 'scene') {
      return;
    }

    const now = performance.now();
    if (lastFrameAt > 0) {
      const delta = now - lastFrameAt;
      if (delta > 0) {
        fps = Math.round(1000 / delta);
      }
    }
    lastFrameAt = now;

    scene = payload.scene;
    metrics = payload.metrics;
    grok = payload.grok ?? { text: '', error: '' };
    if (payload.camera) {
      camera = payload.camera;
    }
  });

  function confidencePct(value) {
    return Math.round((value ?? 0) * 100);
  }

  function confidenceTone(value) {
    const pct = confidencePct(value);
    if (pct >= 75) return 'high';
    if (pct >= 50) return 'mid';
    return 'low';
  }

  function formatMs(value) {
    return `${(value ?? 0).toFixed(1)} ms`;
  }

  function labelIcon(label) {
    const key = (label ?? '').toLowerCase();
    if (key.includes('person') || key.includes('pedestrian')) return 'ped';
    if (key.includes('car') || key.includes('truck') || key.includes('bus')) return 'veh';
    if (key.includes('bike') || key.includes('motor')) return 'bike';
    return 'obj';
  }

  const metricRows = $derived(
    metrics
      ? [
          { key: 'capture', label: 'Capture', value: metrics.capture_ms },
          { key: 'infer', label: 'Inference', value: metrics.infer_ms },
          { key: 'predict', label: 'Prediction', value: metrics.predict_ms },
          { key: 'total', label: 'Total', value: metrics.total_ms, emphasis: true },
        ]
      : [],
  );

  const maxMetric = $derived(
    metricRows.length ? Math.max(...metricRows.map((row) => row.value ?? 0), 1) : 1,
  );

  const detectionCount = $derived(scene?.detections?.length ?? 0);
  const predictionCount = $derived(scene?.predictions?.length ?? 0);
</script>

<div class="panel">
  <header class="topbar">
    <div class="brand">
      <span class="brand-mark" aria-hidden="true"></span>
      <div>
        <p class="brand-name">Fovea</p>
        <p class="brand-sub">Passenger display</p>
      </div>
    </div>

    <div class="topbar-meta">
      {#if scene?.frame_index != null}
        <span class="meta-chip">Frame {scene.frame_index}</span>
      {/if}
      {#if fps > 0}
        <span class="meta-chip">{fps} FPS</span>
      {/if}
      <span class="status-pill" data-live={connected}>
        <span class="status-dot" aria-hidden="true"></span>
        {connected ? 'Live' : 'Offline'}
      </span>
    </div>
  </header>

  <nav class="camera-strip" aria-label="Camera views">
    {#each cameraModes as mode}
      <button
        type="button"
        class="camera-tab"
        data-active={camera.mode === mode.id}
        data-disabled={!mode.available}
        disabled={!mode.available}
        title={mode.available ? mode.label : `${mode.label} — coming soon`}
      >
        <span class="camera-tab-label">{mode.label}</span>
        {#if !mode.available}
          <span class="camera-tab-badge">Soon</span>
        {/if}
      </button>
    {/each}
  </nav>

  {#if !connected}
    <div class="banner banner-wait">
      <p>Waiting for <code>fovea_app --panel</code></p>
      <p class="banner-hint">Start the Qt app, then this panel connects to ws://127.0.0.1:8765</p>
    </div>
  {/if}

  <main class="layout">
    <section class="card scene-card">
      <header class="card-header">
        <div>
          <h2>Scene perception</h2>
          <p class="card-sub">{camera.label ?? 'Forward Camera'}</p>
        </div>
        <div class="stat-group">
          <div class="stat">
            <span class="stat-value">{detectionCount}</span>
            <span class="stat-label">Objects</span>
          </div>
          <div class="stat">
            <span class="stat-value">{predictionCount}</span>
            <span class="stat-label">Predictions</span>
          </div>
        </div>
      </header>

      {#if scene?.detections?.length}
        <ul class="detection-list">
          {#each scene.detections as detection}
            <li class="detection-row">
              <div class="detection-main">
                <span class="detection-icon" data-kind={labelIcon(detection.label)} aria-hidden="true"></span>
                <div>
                  <p class="detection-label">{detection.label}</p>
                  <p class="detection-id">Track #{detection.id}</p>
                </div>
              </div>
              <div class="detection-confidence">
                <div class="confidence-bar" aria-hidden="true">
                  <span
                    class="confidence-fill"
                    data-tone={confidenceTone(detection.confidence)}
                    style={`width: ${confidencePct(detection.confidence)}%`}
                  ></span>
                </div>
                <span class="confidence-value">{confidencePct(detection.confidence)}%</span>
              </div>
            </li>
          {/each}
        </ul>
      {:else}
        <div class="empty-state">
          <p>No objects detected in the current frame.</p>
        </div>
      {/if}
    </section>

    <section class="card metrics-card">
      <header class="card-header">
        <div>
          <h2>Pipeline latency</h2>
          <p class="card-sub">Per-frame processing breakdown</p>
        </div>
        {#if metrics}
          <p class="total-ms">{formatMs(metrics.total_ms)}</p>
        {/if}
      </header>

      {#if metricRows.length}
        <ul class="metric-list">
          {#each metricRows as row}
            <li class="metric-row" data-emphasis={row.emphasis ?? false}>
              <div class="metric-label-row">
                <span>{row.label}</span>
                <span class="metric-value">{formatMs(row.value)}</span>
              </div>
              <div class="metric-track" aria-hidden="true">
                <span
                  class="metric-fill"
                  style={`width: ${Math.min(100, ((row.value ?? 0) / maxMetric) * 100)}%`}
                ></span>
              </div>
            </li>
          {/each}
        </ul>
      {:else}
        <div class="empty-state">
          <p>Metrics appear when frames stream from the pipeline.</p>
        </div>
      {/if}
    </section>

    <section class="card narration-card">
      <header class="card-header">
        <div>
          <h2>Passenger narration</h2>
          <p class="card-sub">Plain-language scene explanation</p>
        </div>
      </header>

      {#if grok.text}
        <p class="narration-text">{grok.text}</p>
      {:else}
        <div class="empty-state compact">
          <p>Run with <code>--grok</code> and <code>XAI_API_KEY</code> for live explanations.</p>
        </div>
      {/if}

      {#if grok.error}
        <p class="narration-error" role="alert">{grok.error}</p>
      {/if}
    </section>
  </main>

  <footer class="footer">
    <p>v0.6 · Forward dashcam · Reverse &amp; 360° views on the roadmap</p>
  </footer>
</div>

<style>
  .panel {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    background: var(--bg-base);
  }

  .topbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 16px 24px;
    border-bottom: 1px solid var(--border-subtle);
    background: var(--bg-elevated);
  }

  .brand {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .brand-mark {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--accent);
    box-shadow: 0 0 12px var(--accent);
  }

  .brand-name {
    margin: 0;
    font-size: 15px;
    font-weight: 600;
    letter-spacing: -0.01em;
  }

  .brand-sub {
    margin: 2px 0 0;
    font-size: 12px;
    color: var(--text-secondary);
  }

  .topbar-meta {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
    justify-content: flex-end;
  }

  .meta-chip {
    font-size: 12px;
    font-weight: 500;
    color: var(--text-secondary);
    padding: 4px 10px;
    border-radius: 999px;
    background: var(--bg-surface);
    border: 1px solid var(--border-subtle);
    font-family: var(--font-mono);
  }

  .status-pill {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    font-size: 12px;
    font-weight: 600;
    padding: 6px 12px;
    border-radius: 999px;
    color: var(--text-secondary);
    background: var(--bg-surface);
    border: 1px solid var(--border-subtle);
    text-transform: uppercase;
    letter-spacing: 0.06em;
  }

  .status-pill[data-live='true'] {
    color: var(--success);
    background: var(--success-muted);
    border-color: rgba(52, 199, 89, 0.35);
  }

  .status-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--text-tertiary);
  }

  .status-pill[data-live='true'] .status-dot {
    background: var(--success);
    box-shadow: 0 0 8px var(--success);
  }

  .camera-strip {
    display: flex;
    gap: 4px;
    padding: 8px 24px;
    border-bottom: 1px solid var(--border-subtle);
    background: var(--bg-elevated);
    overflow-x: auto;
  }

  .camera-tab {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    padding: 10px 16px;
    border-radius: var(--radius-sm);
    font-size: 14px;
    font-weight: 500;
    color: var(--text-secondary);
    white-space: nowrap;
    transition: background 0.15s, color 0.15s;
  }

  .camera-tab[data-active='true'] {
    color: var(--text-primary);
    background: var(--accent-muted);
    box-shadow: inset 0 0 0 1px rgba(59, 130, 246, 0.35);
  }

  .camera-tab[data-disabled='true'] {
    opacity: 0.55;
    cursor: not-allowed;
  }

  .camera-tab:not([data-disabled='true']):hover {
    background: var(--bg-surface-hover);
  }

  .camera-tab-badge {
    font-size: 10px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    padding: 2px 6px;
    border-radius: 4px;
    color: var(--text-tertiary);
    background: var(--bg-surface);
    border: 1px solid var(--border-subtle);
  }

  .banner {
    margin: 16px 24px 0;
    padding: 14px 16px;
    border-radius: var(--radius-md);
    border: 1px solid var(--border-subtle);
  }

  .banner-wait {
    background: var(--bg-surface);
    color: var(--text-secondary);
  }

  .banner-wait p {
    margin: 0;
    font-size: 14px;
  }

  .banner-hint {
    margin-top: 4px !important;
    font-size: 12px !important;
    color: var(--text-tertiary) !important;
  }

  .layout {
    flex: 1;
    display: grid;
    grid-template-columns: 1.2fr 0.8fr;
    grid-template-rows: auto auto;
    gap: 16px;
    padding: 20px 24px 12px;
  }

  .narration-card {
    grid-column: 1 / -1;
  }

  @media (max-width: 900px) {
    .layout {
      grid-template-columns: 1fr;
    }
  }

  .card {
    background: var(--bg-elevated);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    box-shadow: var(--shadow-panel);
    padding: 20px;
    min-height: 200px;
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    gap: 16px;
    margin-bottom: 16px;
  }

  .card-header h2 {
    margin: 0;
    font-size: 16px;
    font-weight: 600;
    letter-spacing: -0.01em;
  }

  .card-sub {
    margin: 4px 0 0;
    font-size: 13px;
    color: var(--text-secondary);
  }

  .stat-group {
    display: flex;
    gap: 20px;
  }

  .stat {
    text-align: right;
  }

  .stat-value {
    display: block;
    font-size: 22px;
    font-weight: 600;
    font-family: var(--font-mono);
    color: var(--text-primary);
    line-height: 1.1;
  }

  .stat-label {
    font-size: 11px;
    font-weight: 500;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--text-tertiary);
  }

  .total-ms {
    margin: 0;
    font-size: 20px;
    font-weight: 600;
    font-family: var(--font-mono);
    color: var(--accent-text);
  }

  .detection-list,
  .metric-list {
    list-style: none;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .detection-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 12px 14px;
    border-radius: var(--radius-md);
    background: var(--bg-surface);
    border: 1px solid var(--border-subtle);
  }

  .detection-main {
    display: flex;
    align-items: center;
    gap: 12px;
    min-width: 0;
  }

  .detection-icon {
    width: 32px;
    height: 32px;
    border-radius: var(--radius-sm);
    flex-shrink: 0;
    background: var(--bg-surface-hover);
    border: 1px solid var(--border-subtle);
    position: relative;
  }

  .detection-icon::after {
    content: '';
    position: absolute;
    inset: 8px;
    border-radius: 3px;
    background: var(--accent-text);
    opacity: 0.85;
  }

  .detection-icon[data-kind='ped']::after {
    border-radius: 50%;
    background: var(--warning);
  }

  .detection-icon[data-kind='veh']::after {
    background: var(--accent-text);
  }

  .detection-icon[data-kind='bike']::after {
    background: var(--success);
  }

  .detection-label {
    margin: 0;
    font-size: 14px;
    font-weight: 500;
    color: var(--text-primary);
    text-transform: capitalize;
  }

  .detection-id {
    margin: 2px 0 0;
    font-size: 12px;
    color: var(--text-tertiary);
    font-family: var(--font-mono);
  }

  .detection-confidence {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-shrink: 0;
  }

  .confidence-bar {
    width: 72px;
    height: 6px;
    border-radius: 999px;
    background: var(--bg-base);
    overflow: hidden;
  }

  .confidence-fill {
    display: block;
    height: 100%;
    border-radius: inherit;
    background: var(--accent);
  }

  .confidence-fill[data-tone='mid'] {
    background: var(--warning);
  }

  .confidence-fill[data-tone='low'] {
    background: var(--danger);
  }

  .confidence-value {
    font-size: 13px;
    font-weight: 600;
    font-family: var(--font-mono);
    color: var(--text-secondary);
    min-width: 36px;
    text-align: right;
  }

  .metric-row {
    padding: 10px 0;
    border-bottom: 1px solid var(--border-subtle);
  }

  .metric-row:last-child {
    border-bottom: none;
    padding-bottom: 0;
  }

  .metric-row[data-emphasis='true'] .metric-label-row span:first-child {
    color: var(--text-primary);
    font-weight: 600;
  }

  .metric-row[data-emphasis='true'] .metric-value {
    color: var(--accent-text);
  }

  .metric-label-row {
    display: flex;
    justify-content: space-between;
    margin-bottom: 8px;
    font-size: 13px;
    color: var(--text-secondary);
  }

  .metric-value {
    font-family: var(--font-mono);
    font-weight: 500;
    color: var(--text-primary);
  }

  .metric-track {
    height: 4px;
    border-radius: 999px;
    background: var(--bg-surface);
    overflow: hidden;
  }

  .metric-fill {
    display: block;
    height: 100%;
    border-radius: inherit;
    background: linear-gradient(90deg, var(--accent) 0%, var(--accent-text) 100%);
  }

  .narration-text {
    margin: 0;
    font-size: 15px;
    line-height: 1.65;
    color: var(--text-primary);
  }

  .narration-error {
    margin: 12px 0 0;
    padding: 10px 12px;
    border-radius: var(--radius-sm);
    font-size: 13px;
    color: var(--danger);
    background: rgba(255, 69, 58, 0.1);
    border: 1px solid rgba(255, 69, 58, 0.25);
  }

  .empty-state {
    padding: 32px 16px;
    text-align: center;
    color: var(--text-secondary);
    font-size: 14px;
    border-radius: var(--radius-md);
    background: var(--bg-surface);
    border: 1px dashed var(--border-subtle);
  }

  .empty-state.compact {
    padding: 20px 16px;
  }

  .empty-state p {
    margin: 0;
  }

  code {
    font-family: var(--font-mono);
    font-size: 0.92em;
    color: var(--accent-text);
    background: var(--accent-muted);
    padding: 2px 6px;
    border-radius: 4px;
  }

  .footer {
    padding: 12px 24px 20px;
    font-size: 12px;
    color: var(--text-tertiary);
    text-align: center;
  }

  .footer p {
    margin: 0;
  }
</style>
