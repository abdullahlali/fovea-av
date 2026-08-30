<script>
  const wsUrl = import.meta.env.VITE_FOVEA_WS_URL ?? 'ws://127.0.0.1:8765';

  let connected = $state(false);
  let scene = $state(null);
  let grok = $state({ text: '', error: '' });
  let camera = $state({ mode: 'forward', label: 'Forward Camera' });

  const socket = new WebSocket(wsUrl);

  socket.addEventListener('open', () => {
    connected = true;
  });

  socket.addEventListener('close', () => {
    connected = false;
  });

  socket.addEventListener('message', (event) => {
    const payload = JSON.parse(event.data);
    if (payload.type !== 'scene') return;
    scene = payload.scene;
    grok = payload.grok ?? { text: '', error: '' };
    if (payload.camera) camera = payload.camera;
  });

  function confidencePct(value) {
    return Math.round((value ?? 0) * 100);
  }

  const objects = $derived(scene?.detections ?? []);
</script>

<div class="cabin">
  <header class="header">
    <div class="header-left">
      <span class="logo" aria-hidden="true"></span>
      <div>
        <h1>Passenger view</h1>
        <p class="subtitle">Synced with the Fovea driver display</p>
      </div>
    </div>
    <div class="header-right">
      <span class="camera-tag">{camera.label}</span>
      <span class="live" data-on={connected}>
        <span class="live-dot"></span>
        {connected ? 'Connected' : 'Waiting for driver app'}
      </span>
    </div>
  </header>

  {#if !connected}
    <section class="callout">
      <p class="callout-title">Start the driver display first</p>
      <p class="callout-body">
        Run <code>./build/app/fovea_app &lt;video&gt; --panel</code> in another terminal.
        This screen will mirror what the car sees — narration and object list only; the video
        stays on the driver window.
      </p>
    </section>
  {/if}

  <section class="narration" aria-labelledby="narration-heading">
    <h2 id="narration-heading">What the vehicle sees</h2>
    {#if grok.text}
      <p class="narration-body">{grok.text}</p>
    {:else if connected && objects.length > 0}
      <p class="narration-body placeholder">
        Tracking {objects.length} object{objects.length === 1 ? '' : 's'} in the forward view.
        Add <code>--grok</code> for plain-language explanations.
      </p>
    {:else if connected}
      <p class="narration-body placeholder">No objects in the current frame.</p>
    {:else}
      <p class="narration-body placeholder">Narration appears when the driver app is running.</p>
    {/if}
    {#if grok.error}
      <p class="narration-error" role="alert">{grok.error}</p>
    {/if}
  </section>

  <section class="objects" aria-labelledby="objects-heading">
    <div class="objects-head">
      <h2 id="objects-heading">Nearby objects</h2>
      {#if connected}
        <span class="count">{objects.length}</span>
      {/if}
    </div>

    {#if objects.length > 0}
      <ul class="object-list">
        {#each objects as obj}
          <li>
            <span class="object-label">{obj.label}</span>
            <span class="object-meta">Track {obj.id}</span>
            <span class="object-conf">{confidencePct(obj.confidence)}%</span>
          </li>
        {/each}
      </ul>
    {:else}
      <p class="objects-empty">{connected ? 'Clear frame.' : '—'}</p>
    {/if}
  </section>

  <footer class="footer">
    <p>
      <strong>Driver display</strong> — Qt app with camera + boxes &nbsp;·&nbsp;
      <strong>Passenger display</strong> — this page &nbsp;·&nbsp;
      Reverse &amp; 360° coming v0.7+
    </p>
  </footer>
</div>

<style>
  .cabin {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    max-width: 720px;
    margin: 0 auto;
    padding: 0 20px 24px;
  }

  .header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    gap: 16px;
    padding: 24px 0 20px;
    border-bottom: 1px solid var(--border-subtle);
  }

  .header-left {
    display: flex;
    gap: 14px;
    align-items: flex-start;
  }

  .logo {
    width: 8px;
    height: 32px;
    border-radius: 4px;
    background: var(--accent);
    flex-shrink: 0;
    margin-top: 4px;
  }

  h1 {
    margin: 0;
    font-size: 20px;
    font-weight: 600;
    letter-spacing: -0.02em;
  }

  h2 {
    margin: 0 0 12px;
    font-size: 13px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--text-secondary);
  }

  .subtitle {
    margin: 4px 0 0;
    font-size: 14px;
    color: var(--text-secondary);
  }

  .header-right {
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    gap: 8px;
  }

  .camera-tag {
    font-size: 12px;
    color: var(--text-secondary);
    padding: 4px 10px;
    border-radius: 6px;
    background: var(--bg-surface);
    border: 1px solid var(--border-subtle);
  }

  .live {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    font-size: 12px;
    font-weight: 500;
    color: var(--text-tertiary);
  }

  .live[data-on='true'] {
    color: var(--success);
  }

  .live-dot {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--text-tertiary);
  }

  .live[data-on='true'] .live-dot {
    background: var(--success);
    box-shadow: 0 0 6px var(--success);
  }

  .callout {
    margin-top: 20px;
    padding: 16px 18px;
    border-radius: var(--radius-md);
    background: var(--bg-elevated);
    border: 1px solid var(--border-subtle);
  }

  .callout-title {
    margin: 0 0 6px;
    font-size: 15px;
    font-weight: 600;
  }

  .callout-body {
    margin: 0;
    font-size: 14px;
    line-height: 1.55;
    color: var(--text-secondary);
  }

  .narration {
    flex: 1;
    margin-top: 28px;
  }

  .narration-body {
    margin: 0;
    font-size: 22px;
    font-weight: 400;
    line-height: 1.5;
    letter-spacing: -0.02em;
    color: var(--text-primary);
  }

  .narration-body.placeholder {
    font-size: 18px;
    color: var(--text-secondary);
  }

  .narration-error {
    margin: 16px 0 0;
    font-size: 14px;
    color: var(--danger);
  }

  .objects {
    margin-top: 32px;
    padding-top: 24px;
    border-top: 1px solid var(--border-subtle);
  }

  .objects-head {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-bottom: 12px;
  }

  .objects-head h2 {
    margin: 0;
  }

  .count {
    font-size: 12px;
    font-weight: 600;
    font-family: var(--font-mono);
    color: var(--text-secondary);
    padding: 2px 8px;
    border-radius: 999px;
    background: var(--bg-surface);
    border: 1px solid var(--border-subtle);
  }

  .object-list {
    list-style: none;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .object-list li {
    display: grid;
    grid-template-columns: 1fr auto auto;
    gap: 12px;
    align-items: center;
    padding: 12px 14px;
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    border: 1px solid var(--border-subtle);
  }

  .object-label {
    font-size: 15px;
    font-weight: 500;
    text-transform: capitalize;
    color: var(--text-primary);
  }

  .object-meta {
    font-size: 12px;
    font-family: var(--font-mono);
    color: var(--text-tertiary);
  }

  .object-conf {
    font-size: 14px;
    font-weight: 600;
    font-family: var(--font-mono);
    color: var(--text-secondary);
    min-width: 40px;
    text-align: right;
  }

  .objects-empty {
    margin: 0;
    font-size: 14px;
    color: var(--text-tertiary);
  }

  .footer {
    margin-top: auto;
    padding-top: 28px;
    font-size: 12px;
    line-height: 1.5;
    color: var(--text-tertiary);
    text-align: center;
  }

  .footer p {
    margin: 0;
  }

  .footer strong {
    color: var(--text-secondary);
    font-weight: 500;
  }

  code {
    font-family: var(--font-mono);
    font-size: 0.9em;
    color: var(--accent-text);
    background: var(--accent-muted);
    padding: 2px 6px;
    border-radius: 4px;
  }
</style>
