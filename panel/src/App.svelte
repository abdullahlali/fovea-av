<script>
  import { mergeStableTracks, passengerBriefing, objectDistanceLabel } from './lib/passenger.js';

  const wsUrl = import.meta.env.VITE_FOVEA_WS_URL ?? 'ws://127.0.0.1:8765';

  const cameras = [
    { id: 'forward', label: 'Forward', sub: 'Primary road view', active: true },
    { id: 'reverse', label: 'Reverse', sub: 'Backup camera', active: false },
    { id: 'surround', label: 'Surround', sub: '360° view', active: false },
  ];

  let connected = $state(false);
  let scene = $state(null);
  let metrics = $state(null);
  let grok = $state({ text: '', error: '' });
  let camera = $state({ mode: 'forward', label: 'Forward Camera' });
  let stableTracks = $state(new Map());

  const socket = new WebSocket(wsUrl);

  socket.addEventListener('open', () => {
    connected = true;
  });

  socket.addEventListener('close', () => {
    connected = false;
    stableTracks = new Map();
  });

  socket.addEventListener('message', (event) => {
    const payload = JSON.parse(event.data);
    if (payload.type !== 'scene') return;
    scene = payload.scene;
    metrics = payload.metrics;
    grok = payload.grok ?? { text: '', error: '' };
    if (payload.camera) camera = payload.camera;

    const incoming = payload.scene?.detections ?? [];
    stableTracks = mergeStableTracks(stableTracks, incoming);
  });

  function pct(value) {
    return Math.round((value ?? 0) * 100);
  }

  function kind(label) {
    const k = (label ?? '').toLowerCase();
    if (k.includes('person') || k.includes('pedestrian')) return 'person';
    if (k.includes('truck') || k.includes('bus')) return 'truck';
    if (k.includes('car') || k.includes('vehicle')) return 'car';
    if (k.includes('bike') || k.includes('motor')) return 'bike';
    return 'object';
  }

  const objects = $derived(
    [...stableTracks.values()].sort((a, b) => b.confidence - a.confidence),
  );

  const activeCount = $derived(objects.filter((o) => !o.fading).length);

  const briefing = $derived(passengerBriefing(objects, grok.text));

  const responseMs = $derived(metrics ? Math.round(metrics.total_ms) : null);
</script>

<div class="display">
  <header class="top">
    <div class="brand-block">
      <div class="brand-mark" aria-hidden="true">
        <svg viewBox="0 0 24 24" fill="none">
          <circle cx="12" cy="12" r="9" stroke="currentColor" stroke-width="1.5" />
          <circle cx="12" cy="12" r="3" fill="currentColor" />
        </svg>
      </div>
      <div>
        <p class="brand">FOVEA</p>
        <p class="brand-role">Passenger display</p>
      </div>
    </div>

    <div class="top-center">
      <span class="chip">{camera.label}</span>
      {#if responseMs != null}
        <span class="chip">Updates every {responseMs} ms</span>
      {/if}
    </div>

    <div class="top-right">
      <div class="status" data-live={connected}>
        <span class="status-led"></span>
        <span>{connected ? 'System live' : 'Awaiting vehicle'}</span>
      </div>
    </div>
  </header>

  <div class="body">
    <aside class="rail" aria-label="Camera views">
      <p class="rail-title">Views</p>
      {#each cameras as cam}
        <button
          type="button"
          class="rail-item"
          data-active={camera.mode === cam.id && cam.active}
          data-disabled={!cam.active}
          disabled={!cam.active}
        >
          <span class="rail-icon" data-kind={cam.id} aria-hidden="true"></span>
          <span class="rail-copy">
            <span class="rail-label">{cam.label}</span>
            <span class="rail-sub">{cam.active ? cam.sub : 'Coming soon'}</span>
          </span>
        </button>
      {/each}

      <div class="rail-foot">
        <p class="rail-foot-label">Perception</p>
        <p class="rail-foot-value" data-on={connected && activeCount > 0}>
          {connected ? (activeCount > 0 ? 'Tracking' : 'Scanning') : 'Offline'}
        </p>
      </div>
    </aside>

    <main class="main">
      {#if !connected}
        <section class="hero-card offline-card">
          <div class="offline-icon" aria-hidden="true">
            <svg viewBox="0 0 64 64" fill="none">
              <rect x="8" y="20" width="48" height="28" rx="6" stroke="currentColor" stroke-width="2" />
              <path d="M20 20 L26 12 H38 L44 20" stroke="currentColor" stroke-width="2" />
              <circle cx="20" cy="48" r="4" fill="currentColor" />
              <circle cx="44" cy="48" r="4" fill="currentColor" />
            </svg>
          </div>
          <h1>Connect to the driver display</h1>
          <p>
            Start <code>fovea_app</code> with <code>--panel</code>. This screen receives live perception
            data from the same pipeline that powers the center display.
          </p>
        </section>
      {:else}
        <section class="hero-card">
          <div class="hero-head">
            <div class="hero-copy">
              <p class="eyebrow">Autonomy briefing</p>
              <h1>What the vehicle sees</h1>
            </div>
            <div class="hero-stat">
              <span class="hero-stat-value">{activeCount}</span>
              <span class="hero-stat-label">Nearby</span>
            </div>
          </div>

          <p class="briefing">{briefing}</p>

          {#if grok.error}
            <p class="briefing-error" role="alert">{grok.error}</p>
          {/if}
        </section>
      {/if}

      <section class="objects-section">
        <div class="section-head">
          <h2>Around you</h2>
          <span class="section-meta">
            {connected ? (activeCount > 0 ? `${activeCount} nearby` : 'Path clear') : 'No data'}
          </span>
        </div>

        {#if objects.length > 0}
          <div class="object-grid">
            {#each objects as obj (obj.id)}
              <article class="object-card" data-fading={obj.fading}>
                <div class="object-icon" data-kind={kind(obj.label)} aria-hidden="true">
                  {#if kind(obj.label) === 'person'}
                    <svg viewBox="0 0 24 24"><circle cx="12" cy="7" r="3.5" fill="currentColor" /><path d="M5 20c0-4 3.5-6 7-6s7 2 7 6" fill="currentColor" /></svg>
                  {:else if kind(obj.label) === 'car' || kind(obj.label) === 'truck'}
                    <svg viewBox="0 0 24 24"><path d="M4 14h16l-1.5-5H6L4 14z" fill="currentColor" /><rect x="3" y="14" width="18" height="5" rx="1" fill="currentColor" opacity="0.7" /><circle cx="7" cy="19" r="1.5" fill="var(--bg-card)" /><circle cx="17" cy="19" r="1.5" fill="var(--bg-card)" /></svg>
                  {:else}
                    <svg viewBox="0 0 24 24"><rect x="5" y="5" width="14" height="14" rx="3" fill="currentColor" /></svg>
                  {/if}
                </div>
                <div class="object-body">
                  <p class="object-name">{obj.label}</p>
                  <p class="object-track">{objectDistanceLabel()}</p>
                </div>
                <div class="object-metric">
                  <span class="object-pct">{pct(obj.confidence)}%</span>
                  <span class="object-pct-label">sure</span>
                </div>
                <div class="object-bar" aria-hidden="true">
                  <span style={`width: ${pct(obj.confidence)}%`}></span>
                </div>
              </article>
            {/each}
          </div>
        {:else}
          <div class="empty-card">
            <p>{connected ? 'Nothing detected nearby right now.' : 'Objects appear when the vehicle is connected.'}</p>
          </div>
        {/if}
      </section>

      {#if metrics && connected && responseMs != null}
        <footer class="status-foot">
          <p>
            <strong>System response</strong> · {responseMs} ms
            <span class="status-foot-note">How quickly the vehicle refreshes what it sees</span>
          </p>
          <p class="status-foot-hint">
            Trajectory previews appear on the driver display, not here.
          </p>
        </footer>
      {/if}
    </main>
  </div>
</div>

<style>
  .display {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    background:
      radial-gradient(ellipse 80% 50% at 50% -20%, rgba(62, 106, 225, 0.08), transparent),
      var(--bg-root);
  }

  .top {
    display: grid;
    grid-template-columns: 1fr auto 1fr;
    align-items: center;
    gap: 16px;
    padding: 18px 28px;
    background: var(--bg-shell);
    border-bottom: 1px solid var(--line);
  }

  .brand-block {
    display: flex;
    align-items: center;
    gap: 14px;
  }

  .brand-mark {
    width: 40px;
    height: 40px;
    display: grid;
    place-items: center;
    border-radius: var(--radius-control);
    background: var(--brand-soft);
    color: var(--brand);
    box-shadow: 0 0 24px var(--brand-glow);
  }

  .brand-mark svg {
    width: 22px;
    height: 22px;
  }

  .brand {
    margin: 0;
    font-size: 15px;
    font-weight: 700;
    letter-spacing: 0.22em;
  }

  .brand-role {
    margin: 2px 0 0;
    font-size: 12px;
    color: var(--text-muted);
    font-weight: 500;
  }

  .top-center {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
    justify-content: center;
  }

  .chip {
    font-size: var(--text-caption);
    font-weight: 500;
    padding: 7px 14px;
    border-radius: var(--radius-pill);
    background: var(--bg-card);
    border: 1px solid var(--line);
    color: var(--text-secondary);
  }

  .chip.mono {
    font-weight: 500;
  }

  .top-right {
    display: flex;
    justify-content: flex-end;
  }

  .status {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    padding: 8px 16px;
    border-radius: var(--radius-pill);
    font-size: 13px;
    font-weight: 600;
    color: var(--text-muted);
    background: var(--bg-card);
    border: 1px solid var(--line);
  }

  .status[data-live='true'] {
    color: var(--live);
    background: var(--live-soft);
    border-color: rgba(61, 214, 140, 0.25);
  }

  .status-led {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--text-muted);
  }

  .status[data-live='true'] .status-led {
    background: var(--live);
    box-shadow: 0 0 10px var(--live);
  }

  .body {
    flex: 1;
    display: grid;
    grid-template-columns: 240px 1fr;
    min-height: 0;
  }

  @media (max-width: 860px) {
    .body {
      grid-template-columns: 1fr;
    }

    .rail {
      flex-direction: row !important;
      flex-wrap: wrap;
      border-right: none !important;
      border-bottom: 1px solid var(--line);
    }

    .rail-title,
    .rail-foot {
      display: none;
    }
  }

  .rail {
    display: flex;
    flex-direction: column;
    gap: 6px;
    padding: 20px 14px;
    background: var(--bg-rail);
    border-right: 1px solid var(--line);
  }

  .rail-title {
    margin: 0 0 8px 12px;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.12em;
    text-transform: uppercase;
    color: var(--text-muted);
  }

  .rail-item {
    display: flex;
    align-items: center;
    gap: 12px;
    width: 100%;
    text-align: left;
    padding: 12px;
    border-radius: var(--radius-control);
    border: 1px solid transparent;
    transition: background 0.15s, border-color 0.15s;
  }

  .rail-item[data-active='true'] {
    background: var(--brand-soft);
    border-color: rgba(62, 106, 225, 0.35);
  }

  .rail-item[data-disabled='true'] {
    opacity: 0.45;
    cursor: not-allowed;
  }

  .rail-item:not([data-disabled='true']):hover {
    background: var(--bg-card);
  }

  .rail-icon {
    width: 36px;
    height: 36px;
    border-radius: 10px;
    background: var(--bg-card);
    border: 1px solid var(--line);
    flex-shrink: 0;
    position: relative;
  }

  .rail-icon[data-kind='forward']::after {
    content: '';
    position: absolute;
    inset: 10px 8px;
    border-top: 2px solid var(--brand);
    border-left: 2px solid var(--brand);
    border-right: 2px solid var(--brand);
    border-radius: 4px 4px 0 0;
  }

  .rail-copy {
    display: flex;
    flex-direction: column;
    gap: 2px;
    min-width: 0;
  }

  .rail-label {
    font-size: 14px;
    font-weight: 600;
    color: var(--text-primary);
  }

  .rail-sub {
    font-size: 11px;
    color: var(--text-muted);
  }

  .rail-foot {
    margin-top: auto;
    padding: 16px 12px 4px;
    border-top: 1px solid var(--line);
  }

  .rail-foot-label {
    margin: 0;
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 0.1em;
    color: var(--text-muted);
    font-weight: 600;
  }

  .rail-foot-value {
    margin: 6px 0 0;
    font-size: 18px;
    font-weight: 700;
    color: var(--text-secondary);
  }

  .rail-foot-value[data-on='true'] {
    color: var(--live);
  }

  .main {
    padding: 24px 28px 32px;
    display: flex;
    flex-direction: column;
    gap: 20px;
    overflow-y: auto;
  }

  .hero-card {
    padding: 28px 32px;
    border-radius: var(--radius-card);
    background: var(--bg-card);
    box-shadow: var(--shadow-card);
    border: 1px solid var(--line);
  }

  .offline-card {
    text-align: center;
    padding: 48px 32px;
  }

  .offline-icon {
    width: 64px;
    height: 64px;
    margin: 0 auto 20px;
    color: var(--text-muted);
  }

  .offline-card h1 {
    margin: 0 0 12px;
    font-size: 24px;
    font-weight: 700;
    letter-spacing: -0.02em;
  }

  .offline-card p {
    margin: 0 auto;
    max-width: 480px;
    font-size: 15px;
    line-height: 1.6;
    color: var(--text-secondary);
  }

  .hero-head {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    gap: 24px;
    margin-bottom: 18px;
  }

  .hero-copy {
    max-width: 36rem;
  }

  .eyebrow {
    margin: 0 0 8px;
    font-size: var(--text-micro);
    font-weight: 600;
    letter-spacing: 0.14em;
    text-transform: uppercase;
    color: var(--brand);
  }

  .hero-head h1 {
    margin: 0;
    font-size: var(--text-display);
    font-weight: 600;
    letter-spacing: -0.025em;
    line-height: 1.25;
  }

  .hero-stat {
    text-align: right;
    flex-shrink: 0;
  }

  .hero-stat-value {
    display: block;
    font-size: 2.25rem;
    font-weight: 600;
    line-height: 1;
    color: var(--text-primary);
    font-variant-numeric: tabular-nums;
  }

  .hero-stat-label {
    font-size: var(--text-caption);
    color: var(--text-muted);
    font-weight: 500;
  }

  .briefing {
    margin: 0;
    font-size: 1.125rem;
    line-height: 1.65;
    font-weight: 400;
    letter-spacing: -0.01em;
    color: var(--text-secondary);
    max-width: 42rem;
  }

  .briefing-error {
    margin: 16px 0 0;
    padding: 12px 14px;
    border-radius: var(--radius-control);
    font-size: 14px;
    color: #fecaca;
    background: rgba(239, 68, 68, 0.1);
    border: 1px solid rgba(239, 68, 68, 0.25);
  }

  .objects-section {
    display: flex;
    flex-direction: column;
    gap: 14px;
  }

  .section-head {
    display: flex;
    align-items: baseline;
    justify-content: space-between;
    gap: 12px;
  }

  .section-head h2 {
    margin: 0;
    font-size: var(--text-title);
    font-weight: 600;
    letter-spacing: -0.015em;
  }

  .section-meta {
    font-size: 13px;
    color: var(--text-muted);
    font-weight: 500;
  }

  .object-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
    gap: 12px;
  }

  .object-card {
    position: relative;
    display: grid;
    grid-template-columns: auto 1fr auto;
    grid-template-rows: auto auto;
    gap: 4px 14px;
    padding: 18px 18px 22px;
    border-radius: var(--radius-card);
    background: var(--bg-card);
    border: 1px solid var(--line);
    box-shadow: var(--shadow-inset);
    overflow: hidden;
    transition: opacity 0.35s ease;
  }

  .object-card[data-fading='true'] {
    opacity: 0.55;
  }

  .object-icon {
    grid-row: span 2;
    width: 44px;
    height: 44px;
    display: grid;
    place-items: center;
    border-radius: 12px;
    background: var(--bg-card-raised);
    border: 1px solid var(--line);
    color: var(--text-secondary);
  }

  .object-icon svg {
    width: 24px;
    height: 24px;
  }

  .object-icon[data-kind='person'] {
    color: #fbbf24;
    background: rgba(251, 191, 36, 0.08);
  }

  .object-icon[data-kind='car'],
  .object-icon[data-kind='truck'] {
    color: var(--brand);
    background: var(--brand-soft);
  }

  .object-name {
    margin: 0;
    font-size: var(--text-body);
    font-weight: 600;
    text-transform: capitalize;
    color: var(--text-primary);
  }

  .object-track {
    margin: 2px 0 0;
    font-size: var(--text-caption);
    color: var(--text-muted);
    font-weight: 400;
  }

  .object-metric {
    grid-row: span 2;
    text-align: right;
    align-self: center;
  }

  .object-pct {
    display: block;
    font-size: 1.25rem;
    font-weight: 600;
    color: var(--text-primary);
    line-height: 1;
    font-variant-numeric: tabular-nums;
  }

  .object-pct-label {
    font-size: var(--text-micro);
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--text-muted);
    font-weight: 500;
  }

  .object-bar {
    grid-column: 1 / -1;
    height: 3px;
    border-radius: 999px;
    background: var(--bg-inset);
    overflow: hidden;
    margin-top: 8px;
  }

  .object-bar span {
    display: block;
    height: 100%;
    border-radius: inherit;
    background: linear-gradient(90deg, var(--brand), #6b9aff);
  }

  .empty-card {
    padding: 32px;
    text-align: center;
    border-radius: var(--radius-card);
    background: var(--bg-card);
    border: 1px dashed var(--line-strong);
  }

  .empty-card p {
    margin: 0;
    font-size: 14px;
    color: var(--text-muted);
  }

  .status-foot {
    padding: 16px 20px;
    border-radius: var(--radius-card);
    background: var(--bg-card);
    border: 1px solid var(--line);
  }

  .status-foot p {
    margin: 0;
    font-size: var(--text-caption);
    color: var(--text-secondary);
    line-height: 1.5;
  }

  .status-foot strong {
    color: var(--text-primary);
    font-weight: 600;
  }

  .status-foot-note {
    display: block;
    margin-top: 4px;
    font-size: var(--text-micro);
    color: var(--text-muted);
  }

  .status-foot-hint {
    margin-top: 10px !important;
    font-size: var(--text-micro);
    color: var(--text-muted);
  }

  .mono {
    font-variant-numeric: tabular-nums;
  }

  code {
    font-size: 0.9em;
    padding: 2px 7px;
    border-radius: 6px;
    background: var(--bg-inset);
    border: 1px solid var(--line);
    color: var(--text-secondary);
  }

  @media (max-width: 640px) {
    .top {
      grid-template-columns: 1fr;
      text-align: center;
    }

    .top-right,
    .brand-block {
      justify-content: center;
    }
  }
</style>
