/**
 * Ego-centric bird's-eye scene model.
 *
 * Lanes (x, meters): oncoming (−2) | left (−1) | YOU (0) | right (+1)
 * Image center ≈ your lane; left/right of frame → left/right/oncoming.
 */

export function objectKind(label = '') {
  const k = label.toLowerCase();
  if (k.includes('person') || k.includes('pedestrian')) return 'person';
  if (k.includes('truck') || k.includes('bus')) return 'truck';
  if (k.includes('bike') || k.includes('motor')) return 'bike';
  if (k.includes('car') || k.includes('vehicle')) return 'car';
  return 'object';
}

export function displayLabel(label = '') {
  const k = label.toLowerCase();
  if (k.includes('truck') || k.includes('bus')) return 'Truck';
  if (k.includes('person') || k.includes('pedestrian')) return 'Person';
  if (k.includes('bike') || k.includes('motor')) return 'Bike';
  if (k.includes('car') || k.includes('vehicle')) return 'Car';
  return label || 'Object';
}

const clamp = (v, lo, hi) => Math.min(hi, Math.max(lo, v));
const lerp = (a, b, t) => a + (b - a) * t;

export const SCENE = {
  w: 720,
  h: 400,
  egoX: 360,
  egoY: 342,
  egoFront: 290,
  roadTop: 28,
  laneW: 72,
  get roadHalf() {
    return this.laneW * 2;
  },
  behindPad: 18,
  behindDepth: 52,
};

/** 3D world meters. X=right, Y=up, Z=ahead. */
export const WORLD = {
  laneW: 3.6,
  maxAheadM: 50,
  maxBehindM: 16,
  roadLenM: 70,
  /** oncoming | left | ego | right */
  get roadLeftM() {
    return -this.laneW * 2.55;
  },
  get roadRightM() {
    return this.laneW * 1.55;
  },
  get roadWidthM() {
    return this.roadRightM - this.roadLeftM;
  },
};

const REAL_HEIGHT_M = {
  person: 1.7,
  bike: 1.4,
  car: 1.5,
  truck: 3.2,
  object: 1.5,
};

const SIZE_M = {
  person: { w: 0.55, l: 0.55 },
  bike: { w: 0.7, l: 1.8 },
  car: { w: 1.85, l: 4.4 },
  truck: { w: 2.45, l: 7.5 },
  object: { w: 1.5, l: 3.0 },
};

const smoothState = new Map();
const approachState = new Map(); // id → last bbox height for oncoming heuristic

function imageSize(scene) {
  return { w: scene?.image?.width ?? 1280, h: scene?.image?.height ?? 720 };
}

/**
 * Continuous lateral in lane units from image X.
 * −2 oncoming, −1 left, 0 ego, +1 right
 */
function lateralFromImage(cx, imgW) {
  const nx = (cx / imgW - 0.5) * 2; // -1..1
  // Dashcam FOV ≈ covers opposite + travel lanes
  return clamp(nx * 2.05, -2.35, 1.45);
}

function softLaneSnap(laneUnits) {
  const centers = [-2, -1, 0, 1];
  let best = 0;
  let bestD = Infinity;
  for (const c of centers) {
    const d = Math.abs(laneUnits - c);
    if (d < bestD) {
      bestD = d;
      best = c;
    }
  }
  if (bestD < 0.38) return lerp(laneUnits, best, 0.55);
  return laneUnits;
}

function worldDims(kind) {
  const size = SIZE_M[kind] ?? SIZE_M.object;
  const h = REAL_HEIGHT_M[kind] ?? REAL_HEIGHT_M.object;
  return { w: size.w, l: size.l, h };
}

function detectOncoming(detection, lane, imgH) {
  const bh = Math.max(1, detection.bbox?.height ?? 1);
  const id = detection.id;
  const prevH = approachState.get(id);
  approachState.set(id, bh);
  const growing = prevH != null && bh > prevH * 1.04;
  const vy = detection.velocity?.dy ?? 0;
  // Oncoming: clearly left of ego AND (far left lane OR approaching in image)
  if (lane > -0.85) return false;
  if (lane <= -1.55) return true;
  return growing || vy > 18;
}

function smoothPose(id, next, alpha = 0.32) {
  const prev = smoothState.get(id);
  if (!prev) {
    smoothState.set(id, { ...next });
    return next;
  }
  const blend = (a, b) => a * (1 - alpha) + b * alpha;
  const out = {
    ...next,
    sceneX: blend(prev.sceneX, next.sceneX),
    sceneY: blend(prev.sceneY, next.sceneY),
    sceneW: blend(prev.sceneW, next.sceneW),
    sceneL: blend(prev.sceneL, next.sceneL),
    lane: blend(prev.lane, next.lane),
    range: blend(prev.range, next.range),
    worldX: blend(prev.worldX, next.worldX),
    worldZ: blend(prev.worldZ, next.worldZ),
    worldW: blend(prev.worldW, next.worldW),
    worldL: blend(prev.worldL, next.worldL),
    worldH: blend(prev.worldH, next.worldH),
    imgVx: blend(prev.imgVx ?? 0, next.imgVx ?? 0),
    imgVy: blend(prev.imgVy ?? 0, next.imgVy ?? 0),
    bboxBottom: blend(prev.bboxBottom ?? next.bboxBottom ?? 0, next.bboxBottom ?? 0),
    bboxH: blend(prev.bboxH ?? next.bboxH ?? 0, next.bboxH ?? 0),
    kind: prev.kind === 'truck' || next.kind === 'truck' ? 'truck' : next.kind,
    label: next.label,
    oncoming: next.oncoming,
  };
  // Sticky oncoming flag
  if (prev.oncoming && next.lane < -0.5) out.oncoming = true;
  smoothState.set(id, out);
  return out;
}

export function projectForward(detection, scene) {
  const { w: imgW, h: imgH } = imageSize(scene);
  const bbox = detection.bbox ?? {};
  const bw = Math.max(1, bbox.width ?? 1);
  const bh = Math.max(1, bbox.height ?? 1);
  const cx = (bbox.x ?? 0) + bw / 2;
  const bottom = (bbox.y ?? 0) + bh;
  const kind = objectKind(detection.label);
  const label = displayLabel(detection.label);

  const focal = imgH * 0.95;
  const realH = REAL_HEIGHT_M[kind] ?? REAL_HEIGHT_M.object;
  const distM = clamp((realH * focal) / bh, 5, 100);
  const rangeFromSize = clamp(1 - distM / 80, 0.04, 0.97);
  const bottomT = clamp((bottom - imgH * 0.28) / (imgH * 0.68), 0, 1);
  const range = clamp(rangeFromSize * 0.42 + bottomT * 0.58, 0.04, 0.97);

  let lane = softLaneSnap(lateralFromImage(cx, imgW));
  const oncoming = detectOncoming(detection, lane, imgH);
  if (oncoming && lane > -1.6) lane = Math.min(lane, -1.7);

  const dims = worldDims(kind);
  const worldZ = clamp(distM * 0.5 + range * WORLD.maxAheadM * 0.5, 5, WORLD.maxAheadM);
  const worldX = lane * WORLD.laneW;

  // 2D fallback coords
  const sceneX = SCENE.egoX + (lane / 2) * SCENE.roadHalf;
  const sceneY = SCENE.egoFront - range * (SCENE.egoFront - SCENE.roadTop);
  const sceneW = clamp(dims.w * 12, 16, kind === 'truck' ? 64 : 48);
  const sceneL = clamp(dims.l * 10, 24, kind === 'truck' ? 100 : 70);

  return smoothPose(`fwd:${detection.id}`, {
    id: `fwd:${detection.id}`,
    trackId: detection.id,
    label,
    kind,
    camera: 'forward',
    confidence: detection.confidence ?? 0,
    zone: 'ahead',
    oncoming,
    range,
    lane,
    sceneX,
    sceneY,
    sceneW,
    sceneL,
    worldX,
    worldZ,
    worldW: dims.w,
    worldL: dims.l,
    worldH: dims.h,
    imgVx: detection.velocity?.dx ?? 0,
    imgVy: detection.velocity?.dy ?? 0,
    bboxBottom: bottom,
    bboxH: bh,
    imgH,
    anchor: 'bottom',
  });
}

export function projectReverse(detection, scene) {
  const { w: imgW, h: imgH } = imageSize(scene);
  const bbox = detection.bbox ?? {};
  const bw = Math.max(1, bbox.width ?? 1);
  const bh = Math.max(1, bbox.height ?? 1);
  const cx = (bbox.x ?? 0) + bw / 2;
  const bottom = (bbox.y ?? 0) + bh;
  const kind = objectKind(detection.label);
  const label = displayLabel(detection.label);

  const nearness = clamp((bottom - imgH * 0.25) / (imgH * 0.7), 0.05, 0.98);
  const sizeBoost = clamp(bh / imgH, 0.05, 0.6);
  const closeness = clamp(nearness * 0.65 + sizeBoost * 0.35, 0.08, 0.98);

  let lane = softLaneSnap(lateralFromImage(cx, imgW));
  // Behind: usually same-direction (left/ego/right), not oncoming
  if (lane < -1.6) lane = -1;

  const dims = worldDims(kind);
  const worldZ = -clamp(closeness * WORLD.maxBehindM, 3, WORLD.maxBehindM);
  const worldX = lane * WORLD.laneW;

  return smoothPose(`rev:${detection.id}`, {
    id: `rev:${detection.id}`,
    trackId: detection.id,
    label,
    kind,
    camera: 'reverse',
    confidence: detection.confidence ?? 0,
    zone: 'behind',
    oncoming: false,
    range: closeness,
    lane,
    sceneX: SCENE.egoX + (lane / 2) * SCENE.roadHalf,
    sceneY: SCENE.egoY + SCENE.behindPad + (1 - closeness) * SCENE.behindDepth,
    sceneW: clamp(dims.w * 10, 14, 44),
    sceneL: clamp(dims.l * 5, 16, 36),
    worldX,
    worldZ,
    worldW: dims.w,
    worldL: dims.l * 0.85,
    worldH: dims.h * 0.9,
    imgVx: detection.velocity?.dx ?? 0,
    imgVy: detection.velocity?.dy ?? 0,
    bboxBottom: bottom,
    bboxH: bh,
    imgH,
    anchor: 'top',
  });
}

export function buildSceneModel(forwardObjects, reverseObjects, forwardScene, reverseScene) {
  const activeIds = new Set();

  const ahead = forwardObjects
    .filter((o) => (o.bbox?.width ?? 0) > 4)
    .map((o) => projectForward(o, forwardScene))
    .sort((a, b) => a.range - b.range);

  const behind = reverseObjects
    .filter((o) => (o.bbox?.width ?? 0) > 4)
    .map((o) => projectReverse(o, reverseScene))
    .sort((a, b) => b.range - a.range);

  for (const o of [...ahead, ...behind]) activeIds.add(o.id);
  for (const key of smoothState.keys()) {
    if (!activeIds.has(key)) smoothState.delete(key);
  }
  for (const key of approachState.keys()) {
    if (![...activeIds].some((id) => id.endsWith(`:${key}`) || id.includes(String(key)))) {
      // keep approach state keyed by detection id numbers — prune loosely
    }
  }

  return {
    ahead,
    behind,
    ego: { worldX: 0, worldZ: 0, lane: 0 },
  };
}

export function drivingInstruction(warnings, forwardObjects, reverseObjects, gear = 'D') {
  const top = warnings[0];
  if (top?.level === 'critical') {
    return { title: top.text, detail: top.sub ?? 'Take immediate action.', level: 'critical' };
  }
  if (top?.level === 'warn') {
    return { title: top.text, detail: top.sub ?? 'Adjust speed and position.', level: 'warn' };
  }

  const fwd = forwardObjects.filter((o) => !o.fading);
  const rev = reverseObjects.filter((o) => !o.fading);

  if (gear === 'R') {
    if (rev.length > 0) {
      return {
        title: 'STOP — CHECK BEHIND',
        detail: `${rev.length} object${rev.length > 1 ? 's' : ''} behind you before reversing.`,
        level: 'warn',
      };
    }
    return { title: 'CLEAR TO REVERSE', detail: 'Proceed slowly and keep scanning.', level: 'info' };
  }

  const people = fwd.filter((o) => objectKind(o.label) === 'person');
  if (people.length > 0) {
    return {
      title: 'YIELD TO PEDESTRIAN',
      detail: 'Reduce speed and be prepared to stop.',
      level: 'critical',
    };
  }

  if (fwd.length > 0) {
    return {
      title: 'MAINTAIN DISTANCE',
      detail: `${fwd.length} vehicle${fwd.length > 1 ? 's' : ''} ahead — match traffic speed.`,
      level: 'warn',
    };
  }

  if (rev.length > 0) {
    return {
      title: 'CHECK BLIND SPOTS',
      detail: `${rev.length} object${rev.length > 1 ? 's' : ''} behind the vehicle.`,
      level: 'info',
    };
  }

  return { title: 'PATH CLEAR', detail: 'Continue with normal caution.', level: 'info' };
}
