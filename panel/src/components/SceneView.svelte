<script>
  import { onDestroy, onMount } from 'svelte';
  import * as THREE from 'three';
  import { WORLD } from '../lib/sceneModel.js';

  /** @type {{ ahead: Array<any>, behind: Array<any>, ego?: any }} */
  let { model = { ahead: [], behind: [] }, frameTick = 0 } = $props();

  let host;
  /** @type {THREE.WebGLRenderer | null} */
  let renderer = null;
  /** @type {THREE.Scene | null} */
  let scene = null;
  /** @type {THREE.PerspectiveCamera | null} */
  let camera = null;
  let raf = 0;
  let disposed = false;

  /** @type {Map<string, { group: THREE.Group, kind: string, oncoming: boolean }>} */
  const actors = new Map();
  /** @type {THREE.Group | null} */
  let egoGroup = null;
  /** @type {THREE.Group | null} */
  let roadGroup = null;
  /** @type {THREE.Group | null} */
  let dashGroup = null;

  const DASH_SPACING = 5;
  const DASH_Z_MIN = -12;
  const MAX_ACTORS = 12;

  let dashScroll = 0;
  let egoSpeedMps = 12;
  let lastT = 0;
  let lastFrameTick = -1;
  let lastFrameAt = 0;
  /** @type {Map<string, number>} */
  const prevZ = new Map();

  // Shared materials (huge perf win vs per-mesh Standard+shadows)
  const mats = {
    asphalt: new THREE.MeshBasicMaterial({ color: 0x1a1c22 }),
    shoulder: new THREE.MeshBasicMaterial({ color: 0x12141a }),
    opposite: new THREE.MeshBasicMaterial({ color: 0x151820 }),
    lane: new THREE.MeshBasicMaterial({ color: 0x9ca3af }),
    edge: new THREE.MeshBasicMaterial({ color: 0xffffff, transparent: true, opacity: 0.3 }),
    median: new THREE.MeshBasicMaterial({ color: 0xfbbf24, transparent: true, opacity: 0.55 }),
    ego: new THREE.MeshBasicMaterial({ color: 0xf4f4f5 }),
    glass: new THREE.MeshBasicMaterial({ color: 0x27272a }),
    car: new THREE.MeshBasicMaterial({ color: 0x6b7280 }),
    truck: new THREE.MeshBasicMaterial({ color: 0x8b93a3 }),
    truckCab: new THREE.MeshBasicMaterial({ color: 0xa1a1aa }),
    oncoming: new THREE.MeshBasicMaterial({ color: 0x7c8a9a }),
    person: new THREE.MeshBasicMaterial({ color: 0xf59e0b }),
    bike: new THREE.MeshBasicMaterial({ color: 0x60a5fa }),
    behindTint: new THREE.MeshBasicMaterial({ color: 0x000000, transparent: true, opacity: 0.25 }),
  };

  function box(w, h, d, mat) {
    return new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat);
  }

  function buildVehicle(kind, oncoming) {
    const g = new THREE.Group();
    const bodyMat = oncoming ? mats.oncoming : kind === 'truck' ? mats.truck : kind === 'bike' ? mats.bike : kind === 'person' ? mats.person : mats.car;

    if (kind === 'person') {
      const body = box(0.45, 1.1, 0.35, bodyMat);
      body.position.y = 0.55;
      g.add(body);
    } else if (kind === 'truck') {
      const trailer = box(2.4, 2.5, 5.8, bodyMat);
      trailer.position.set(0, 1.35, -0.5);
      const cab = box(2.4, 2.1, 2.1, oncoming ? mats.oncoming : mats.truckCab);
      cab.position.set(0, 1.15, 3.0);
      g.add(trailer, cab);
    } else if (kind === 'bike') {
      const body = box(0.55, 0.85, 1.7, bodyMat);
      body.position.y = 0.45;
      g.add(body);
    } else {
      const body = box(1.85, 1.3, 4.2, bodyMat);
      body.position.y = 0.65;
      const cabin = box(1.5, 0.45, 1.6, mats.glass);
      cabin.position.set(0, 1.4, -0.15);
      g.add(body, cabin);
    }
    return g;
  }

  function buildEgo() {
    const g = new THREE.Group();
    const body = box(1.9, 1.35, 4.4, mats.ego);
    body.position.y = 0.7;
    const cabin = box(1.5, 0.45, 1.6, mats.glass);
    cabin.position.set(0, 1.45, -0.15);
    g.add(body, cabin);
    return g;
  }

  function buildDashes() {
    const g = new THREE.Group();
    // Same-direction dividers: left|ego and ego|right
    const xs = [-WORLD.laneW * 0.5, WORLD.laneW * 0.5];
    const zMax = WORLD.maxAheadM + 6;
    for (const x of xs) {
      for (let z = DASH_Z_MIN; z < zMax; z += DASH_SPACING) {
        const dash = new THREE.Mesh(new THREE.PlaneGeometry(0.1, 2.0), mats.lane);
        dash.rotation.x = -Math.PI / 2;
        dash.position.set(x, 0.02, z);
        g.add(dash);
      }
    }
    return g;
  }

  function buildRoad() {
    const g = new THREE.Group();
    const len = WORLD.roadLenM;
    const left = WORLD.roadLeftM;
    const right = WORLD.roadRightM;
    const width = right - left;
    const centerX = (left + right) / 2;

    const asphalt = new THREE.Mesh(new THREE.PlaneGeometry(width, len), mats.asphalt);
    asphalt.rotation.x = -Math.PI / 2;
    asphalt.position.set(centerX, 0, len * 0.32);
    g.add(asphalt);

    // Oncoming strip tint (left of median)
    const opp = new THREE.Mesh(
      new THREE.PlaneGeometry(WORLD.laneW * 1.15, len),
      mats.opposite,
    );
    opp.rotation.x = -Math.PI / 2;
    opp.position.set(-WORLD.laneW * 2, 0.005, len * 0.32);
    g.add(opp);

    const ground = new THREE.Mesh(new THREE.PlaneGeometry(70, len + 30), mats.shoulder);
    ground.rotation.x = -Math.PI / 2;
    ground.position.set(0, -0.02, len * 0.28);
    g.add(ground);

    // Solid median between oncoming (−2) and left (−1)
    const median = new THREE.Mesh(new THREE.PlaneGeometry(0.22, len), mats.median);
    median.rotation.x = -Math.PI / 2;
    median.position.set(-WORLD.laneW * 1.5, 0.025, len * 0.32);
    g.add(median);

    // Outer edges
    for (const x of [left, right]) {
      const edge = new THREE.Mesh(new THREE.PlaneGeometry(0.16, len), mats.edge);
      edge.rotation.x = -Math.PI / 2;
      edge.position.set(x, 0.02, len * 0.32);
      g.add(edge);
    }

    const behind = new THREE.Mesh(
      new THREE.PlaneGeometry(WORLD.roadWidthM, WORLD.maxBehindM + 3),
      mats.behindTint,
    );
    behind.rotation.x = -Math.PI / 2;
    behind.position.set(centerX, 0.01, -(WORLD.maxBehindM / 2 + 1));
    g.add(behind);

    return g;
  }

  function footagePlaying() {
    if (frameTick !== lastFrameTick) {
      lastFrameTick = frameTick;
      lastFrameAt = performance.now();
      return true;
    }
    return performance.now() - lastFrameAt < 700;
  }

  function estimateSpeed(dt) {
    if (!footagePlaying()) return 0;
    const objs = model.ahead ?? [];
    let flow = 0;
    let n = 0;
    for (const o of objs) {
      const vy = Math.abs(o.imgVy ?? 0);
      const vx = Math.abs(o.imgVx ?? 0);
      if (vy + vx > 1) {
        flow += vy + 0.3 * vx;
        n += 1;
      }
      const prev = prevZ.get(o.id);
      if (prev != null && o.worldZ != null && dt > 0.001) {
        flow += Math.abs(prev - o.worldZ) / dt * 8;
        n += 1;
      }
      if (o.worldZ != null) prevZ.set(o.id, o.worldZ);
    }
    for (const id of [...prevZ.keys()]) {
      if (!(model.ahead ?? []).some((o) => o.id === id)) prevZ.delete(id);
    }
    if (n === 0) return 13;
    return Math.min(30, Math.max(3, 4 + (flow / n) * 0.12));
  }

  function updateDashes(dt) {
    if (!dashGroup) return;
    const target = estimateSpeed(dt);
    egoSpeedMps += (target - egoSpeedMps) * Math.min(1, dt * 3);
    // Move whole group — one transform instead of N dash updates
    dashScroll += egoSpeedMps * dt;
    dashScroll %= DASH_SPACING;
    if (dashScroll < 0) dashScroll += DASH_SPACING;
    dashGroup.position.z = -dashScroll;
  }

  function syncSize() {
    if (!host || !renderer || !camera) return;
    const w = Math.max(1, host.clientWidth);
    const h = Math.max(1, host.clientHeight);
    renderer.setPixelRatio(1); // fixed 1× — major lag fix in Qt WebEngine
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
  }

  function ensureActor(id, kind, oncoming) {
    let entry = actors.get(id);
    if (entry && (entry.kind !== kind || entry.oncoming !== oncoming)) {
      if (entry.kind === 'truck' || kind === 'truck') {
        entry.kind = 'truck';
      } else {
        scene?.remove(entry.group);
        disposeGroup(entry.group);
        actors.delete(id);
        entry = undefined;
      }
    }
    if (!entry) {
      const group = buildVehicle(kind === 'truck' ? 'truck' : kind, !!oncoming);
      scene?.add(group);
      entry = { group, kind: kind === 'truck' ? 'truck' : kind, oncoming: !!oncoming };
      actors.set(id, entry);
    }
    return entry;
  }

  function disposeGroup(group) {
    group.traverse((obj) => {
      if (obj.geometry) obj.geometry.dispose();
      // shared mats — do not dispose
    });
  }

  function lerp(a, b, t) {
    return a + (b - a) * t;
  }

  function tick(now) {
    if (disposed || !renderer || !scene || !camera) return;
    raf = requestAnimationFrame(tick);

    const dt = lastT ? Math.min(0.05, (now - lastT) / 1000) : 0.016;
    lastT = now;

    updateDashes(dt);

    // Cap actors for perf — nearest first
    const objs = [...(model.ahead ?? []), ...(model.behind ?? [])]
      .sort((a, b) => Math.abs(a.worldZ ?? 99) - Math.abs(b.worldZ ?? 99))
      .slice(0, MAX_ACTORS);
    const liveIds = new Set(objs.map((o) => o.id));

    for (const obj of objs) {
      const entry = ensureActor(obj.id, obj.kind || 'car', obj.oncoming);
      const g = entry.group;
      const tx = obj.worldX ?? 0;
      const tz = obj.worldZ ?? 0;

      if (!g.userData.spawned) {
        g.position.set(tx, 0, tz);
        g.userData.spawned = true;
      }

      const k = 0.2;
      g.position.x = lerp(g.position.x, tx, k);
      g.position.z = lerp(g.position.z, tz, k);

      // Facing: oncoming toward ego; same-direction along +Z
      const yaw = obj.oncoming || obj.zone === 'behind' ? Math.PI : 0;
      // behind same-direction should face +Z (0); fix:
      const face = obj.oncoming ? Math.PI : 0;
      g.rotation.y = lerp(g.rotation.y, face, 0.25);
      void yaw;
    }

    for (const [id, entry] of actors) {
      if (liveIds.has(id)) continue;
      entry.group.userData.miss = (entry.group.userData.miss ?? 0) + 1;
      if (entry.group.userData.miss > 40) {
        scene.remove(entry.group);
        disposeGroup(entry.group);
        actors.delete(id);
      }
    }

    renderer.render(scene, camera);
  }

  onMount(() => {
    disposed = false;
    scene = new THREE.Scene();
    scene.background = new THREE.Color(0x0a0b0e);
    scene.fog = new THREE.Fog(0x0a0b0e, 40, 70);

    camera = new THREE.PerspectiveCamera(40, 1, 0.5, 90);
    camera.position.set(0, 16, -18);
    camera.lookAt(0, 0.4, 8);

    renderer = new THREE.WebGLRenderer({
      antialias: false,
      alpha: false,
      powerPreference: 'high-performance',
    });
    renderer.setClearColor(0x0a0b0e);
    renderer.shadowMap.enabled = false;
    host.appendChild(renderer.domElement);
    Object.assign(renderer.domElement.style, {
      width: '100%',
      height: '100%',
      display: 'block',
      borderRadius: '12px',
    });

    // Cheap fill light only
    scene.add(new THREE.AmbientLight(0xffffff, 1));

    roadGroup = buildRoad();
    dashGroup = buildDashes();
    egoGroup = buildEgo();
    scene.add(roadGroup, dashGroup, egoGroup);

    syncSize();
    const ro = new ResizeObserver(() => syncSize());
    ro.observe(host);
    raf = requestAnimationFrame(tick);

    return () => {
      disposed = true;
      cancelAnimationFrame(raf);
      ro.disconnect();
      for (const [, entry] of actors) {
        disposeGroup(entry.group);
      }
      actors.clear();
      if (egoGroup) disposeGroup(egoGroup);
      if (roadGroup) disposeGroup(roadGroup);
      if (dashGroup) disposeGroup(dashGroup);
      renderer?.dispose();
      renderer?.domElement?.parentNode?.removeChild(renderer.domElement);
      renderer = null;
      scene = null;
      camera = null;
      dashGroup = null;
    };
  });

  onDestroy(() => {
    disposed = true;
    cancelAnimationFrame(raf);
  });
</script>

<div class="scene3d" bind:this={host} aria-label="3D road scene"></div>

<style>
  .scene3d {
    width: 100%;
    height: 100%;
    min-height: 0;
    border-radius: 12px;
    overflow: hidden;
    background: #0a0b0e;
  }
</style>
