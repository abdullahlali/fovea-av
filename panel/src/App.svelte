<script>
  import { onDestroy, onMount } from 'svelte';
  import SceneView from './components/SceneView.svelte';
  import { mergeDualTracks, passengerBriefing, trustPulse } from './lib/passenger.js';
  import { buildSceneModel, drivingInstruction } from './lib/sceneModel.js';
  import { cabinWarnings, gearLabel } from './lib/warnings.js';

  const wsUrl = import.meta.env.VITE_FOVEA_WS_URL ?? 'ws://127.0.0.1:8765';
  const gears = ['P', 'R', 'N', 'D'];
  const SLOT_COUNT = 4;

  let connected = $state(false);
  let forwardScene = $state(null);
  let reverseScene = $state(null);
  let metrics = $state(null);
  let grok = $state({ text: '', error: '' });
  let gear = $state('D');
  let stableTracks = $state(new Map());
  let socket = $state(null);
  let reconnectTimer = null;
  let reconnectDelay = 1500;
  let destroyed = false;
  let clock = $state('');
  let frameTick = $state(0);

  function sendControl(patch) {
    if (!socket || socket.readyState !== WebSocket.OPEN) return;
    socket.send(JSON.stringify({ type: 'control', ...patch }));
  }

  function setGear(next) {
    gear = next;
    sendControl({ gear: next });
  }

  function connect() {
    if (destroyed) return;
    if (socket?.readyState === WebSocket.OPEN || socket?.readyState === WebSocket.CONNECTING) return;
    socket?.close();
    socket = null;

    const ws = new WebSocket(wsUrl);
    socket = ws;
    ws.addEventListener('open', () => { connected = true; reconnectDelay = 1500; });
    ws.addEventListener('close', () => {
      connected = false;
      stableTracks = new Map();
      if (socket === ws) socket = null;
      if (destroyed) return;
      reconnectTimer = setTimeout(() => {
        reconnectDelay = Math.min(reconnectDelay * 1.5, 10000);
        connect();
      }, reconnectDelay);
    });
    ws.addEventListener('error', () => ws.close());
    ws.addEventListener('message', (event) => {
      let payload;
      try { payload = JSON.parse(event.data); } catch { return; }
      if (payload.type !== 'cabin') return;
      forwardScene = payload.forward?.scene ?? null;
      reverseScene = payload.reverse?.scene ?? null;
      metrics = payload.forward?.metrics ?? payload.reverse?.metrics ?? null;
      grok = payload.grok ?? { text: '', error: '' };
      if (payload.gear) gear = payload.gear;
      stableTracks = mergeDualTracks(
        stableTracks,
        payload.forward?.scene?.detections ?? [],
        payload.reverse?.scene?.detections ?? [],
      );
      frameTick += 1;
    });
  }

  onMount(() => {
    destroyed = false;
    updateClock();
    const t = setInterval(updateClock, 30_000);
    connect();
    return () => {
      destroyed = true;
      clearInterval(t);
      if (reconnectTimer) clearTimeout(reconnectTimer);
      socket?.close();
      socket = null;
    };
  });

  onDestroy(() => {
    destroyed = true;
    if (reconnectTimer) clearTimeout(reconnectTimer);
    socket?.close();
    socket = null;
  });

  function updateClock() {
    clock = new Date().toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' });
  }

  function pct(v) { return Math.round((v ?? 0) * 100); }

  const forwardObjects = $derived([...stableTracks.values()].filter((o) => o.camera === 'forward'));
  const reverseObjects = $derived([...stableTracks.values()].filter((o) => o.camera === 'reverse'));
  const displayObjects = $derived(
    [...forwardObjects, ...reverseObjects].filter((o) => !o.fading).sort((a, b) => b.confidence - a.confidence),
  );
  const slots = $derived(Array.from({ length: SLOT_COUNT }, (_, i) => displayObjects[i] ?? null));
  const sceneModel = $derived(buildSceneModel(forwardObjects, reverseObjects, forwardScene, reverseScene));
  const warnings = $derived(cabinWarnings(forwardObjects, reverseObjects, gear));
  const instruction = $derived(drivingInstruction(warnings, forwardObjects, reverseObjects, gear));
  const briefing = $derived(passengerBriefing(forwardObjects, reverseObjects, grok.text, gear));
  const trust = $derived(trustPulse(displayObjects));
  const pipelineMs = $derived(metrics ? Math.round(metrics.total_ms) : null);
</script>

<div class="shell">
  <header class="bar">
    <div class="brand">
      <span class="logo">FOVEA</span>
      <span class="sub">Driver display</span>
    </div>
    <div class="seg" role="group" aria-label="Gear">
      {#each gears as g}
        <button type="button" class="seg-btn" data-on={gear === g} onclick={() => setGear(g)}>{g}</button>
      {/each}
    </div>
    <div class="meta">
      <span>{gearLabel(gear)}</span>
      <span class="live" data-on={connected}>{connected ? 'Live' : 'Offline'}</span>
      <span>{clock}</span>
      {#if pipelineMs}<span>{pipelineMs} ms</span>{/if}
    </div>
  </header>

  <div class="body">
    <section class="scene-panel">
      <SceneView model={sceneModel} frameTick={frameTick} />
      <div class="scene-badge" data-level={trust.level}>{trust.label} · {trust.pct}%</div>
    </section>

    <aside class="side">
      <section class="action" data-level={instruction.level}>
        <p class="action-title">{instruction.title}</p>
        <p class="action-detail">{instruction.detail}</p>
      </section>

      <section class="copilot">
        <p class="side-label">Copilot</p>
        <p class="copilot-text">{briefing}</p>
      </section>

      <section class="objects">
        <div class="objects-head">
          <span class="side-label">Detected</span>
          <span>{displayObjects.length} active</span>
        </div>
        <div class="object-grid">
          {#each slots as obj, i (i)}
            <div class="slot" data-empty={!obj}>
              {#if obj}
                <span class="slot-name">{obj.displayLabel || obj.label}</span>
                <span class="slot-where">
                  {#if obj.camera === 'reverse'}
                    Behind
                  {:else if (obj.bbox?.x ?? 0) + (obj.bbox?.width ?? 0) / 2 < 0.35 * 1280}
                    Left
                  {:else if (obj.bbox?.x ?? 0) + (obj.bbox?.width ?? 0) / 2 > 0.65 * 1280}
                    Right
                  {:else}
                    Ahead
                  {/if}
                </span>
                <span class="slot-pct">{pct(obj.confidence)}%</span>
              {:else}
                <span class="slot-dash">—</span>
              {/if}
            </div>
          {/each}
        </div>
      </section>
    </aside>
  </div>
</div>
