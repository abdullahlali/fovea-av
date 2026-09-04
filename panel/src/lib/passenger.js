/**
 * Stable passenger-side tracks with IoU association + label hysteresis.
 * Fixes truck↔bus flicker when YOLO flips classes on the same vehicle.
 */

import { displayLabel, objectKind } from './sceneModel.js';

const HOLD_MS = 4200;
const CONFIDENCE_BLEND = 0.25;
const IOU_MATCH = 0.22;
const LABEL_FLIP_MARGIN = 0.28;

function iou(a, b) {
  if (!a || !b) return 0;
  const x1 = Math.max(a.x, b.x);
  const y1 = Math.max(a.y, b.y);
  const x2 = Math.min(a.x + a.width, b.x + b.width);
  const y2 = Math.min(a.y + a.height, b.y + b.height);
  const inter = Math.max(0, x2 - x1) * Math.max(0, y2 - y1);
  const union = a.width * a.height + b.width * b.height - inter;
  return union <= 0 ? 0 : inter / union;
}

function sameFamily(a, b) {
  const ka = objectKind(a);
  const kb = objectKind(b);
  if (ka === kb) return true;
  // car vs truck at range — still associate by IoU
  const vehicles = new Set(['car', 'truck', 'bike']);
  return vehicles.has(ka) && vehicles.has(kb);
}

function stabilizeLabel(prev, incoming) {
  if (!prev?.label) return incoming.label;
  if (prev.label === incoming.label) return incoming.label;
  if (!sameFamily(prev.label, incoming.label)) {
    return (incoming.confidence ?? 0) > (prev.confidence ?? 0) + LABEL_FLIP_MARGIN
      ? incoming.label
      : prev.label;
  }
  // Same family (truck/bus): keep locked label unless new is clearly stronger
  if ((incoming.confidence ?? 0) > (prev.confidence ?? 0) + LABEL_FLIP_MARGIN) {
    return incoming.label;
  }
  return prev.label;
}

function trackKey(detection, camera) {
  return `${camera}:${detection.id ?? detection.label}`;
}

export function mergeStableTracks(stable, incoming, camera = 'forward') {
  const now = Date.now();
  const next = new Map();
  const usedIncoming = new Set();

  // Keep tracks from other cameras untouched
  for (const [key, track] of stable) {
    if (!key.startsWith(`${camera}:`)) {
      next.set(key, track);
    }
  }

  const prevTracks = [...stable.entries()].filter(([k]) => k.startsWith(`${camera}:`));

  // 1) Match by detector id when present
  for (const detection of incoming) {
    const key = trackKey(detection, camera);
    const existing = stable.get(key);
    if (!existing) continue;

    usedIncoming.add(detection);
    const label = stabilizeLabel(existing, detection);
    const conf =
      existing.confidence * (1 - CONFIDENCE_BLEND) + (detection.confidence ?? 0) * CONFIDENCE_BLEND;

    next.set(key, {
      ...detection,
      id: existing.id ?? detection.id,
      label,
      displayLabel: displayLabel(label),
      bbox: detection.bbox,
      camera,
      confidence: conf,
      lastSeen: now,
      fading: false,
    });
  }

  // 2) Match remaining by IoU (handles id resets when class flips)
  for (const detection of incoming) {
    if (usedIncoming.has(detection)) continue;

    let best = null;
    let bestIou = 0;
    for (const [key, track] of prevTracks) {
      if (next.has(key)) continue;
      if (!sameFamily(track.label, detection.label) && track.label !== detection.label) continue;
      const score = iou(track.bbox, detection.bbox);
      if (score > bestIou) {
        bestIou = score;
        best = [key, track];
      }
    }

    if (best && bestIou >= IOU_MATCH) {
      const [oldKey, existing] = best;
      usedIncoming.add(detection);
      const label = stabilizeLabel(existing, detection);
      const conf =
        existing.confidence * (1 - CONFIDENCE_BLEND) + (detection.confidence ?? 0) * CONFIDENCE_BLEND;
      const newKey = trackKey({ ...detection, id: existing.id ?? detection.id }, camera);

      next.set(newKey, {
        ...detection,
        id: existing.id ?? detection.id,
        label,
        displayLabel: displayLabel(label),
        bbox: detection.bbox,
        camera,
        confidence: conf,
        lastSeen: now,
        fading: false,
      });
      // drop old key if different
      if (oldKey !== newKey) {
        /* already not copied into next for unmatched */
      }
    } else {
      usedIncoming.add(detection);
      const key = trackKey(detection, camera);
      next.set(key, {
        ...detection,
        label: detection.label,
        displayLabel: displayLabel(detection.label),
        bbox: detection.bbox,
        camera,
        confidence: detection.confidence ?? 0,
        lastSeen: now,
        fading: false,
      });
    }
  }

  // 3) Fade / drop unmatched previous tracks for this camera
  for (const [key, track] of prevTracks) {
    if (next.has(key)) continue;
    // Also skip if we rematched under a new key with same id
    const rematched = [...next.values()].some(
      (t) => t.camera === camera && t.id != null && t.id === track.id && !t.fading,
    );
    if (rematched) continue;

    if (now - track.lastSeen > HOLD_MS) continue;
    next.set(key, { ...track, fading: true });
  }

  return next;
}

export function mergeDualTracks(stable, forwardIncoming, reverseIncoming) {
  let next = mergeStableTracks(stable, forwardIncoming, 'forward');
  next = mergeStableTracks(next, reverseIncoming, 'reverse');
  return next;
}

export function normalizeGrokText(text) {
  if (!text) return '';
  const trimmed = text.replace(/^\[offline mode\]\s*/i, '').trim();
  if (!trimmed) return '';
  if (trimmed.startsWith('{') && trimmed.includes('"error"')) return '';
  if (/grok unavailable/i.test(trimmed)) return '';
  if (/using local passenger briefing/i.test(trimmed)) return '';
  return trimmed;
}

function grokContradictsDetections(narration, fwd, rev) {
  if (fwd.length + rev.length === 0) return false;
  return /\b(clear|calm|normal|enjoy|no obstacles|nothing|empty|all good)\b/i.test(narration);
}

export function passengerBriefing(forwardObjects, reverseObjects, grokText, gear = 'D') {
  const fwd = forwardObjects.filter((o) => !o.fading);
  const rev = reverseObjects.filter((o) => !o.fading);
  const narration = normalizeGrokText(grokText);

  if (narration && !grokContradictsDetections(narration, fwd, rev)) {
    return narration;
  }

  if (gear === 'R') {
    if (rev.length === 0) return 'Area behind is clear. Reverse slowly and keep scanning mirrors.';
    const labels = rev.slice(0, 3).map((o) => o.displayLabel || displayLabel(o.label)).join(', ');
    return `Reversing — ${rev.length} object${rev.length > 1 ? 's' : ''} behind (${labels}). Stop if path is not clear.`;
  }

  if (fwd.length === 0 && rev.length === 0) {
    return 'No obstacles detected. Maintain lane position and normal following distance.';
  }

  const parts = [];
  if (fwd.length > 0) {
    const lead = fwd[0];
    const cx = lead.bbox ? lead.bbox.x + lead.bbox.width / 2 : 640;
    const side = cx < 0.38 * 1280 ? 'left lane' : cx > 0.62 * 1280 ? 'right lane' : 'your lane';
    parts.push(`${fwd.length} ahead including ${lead.displayLabel || displayLabel(lead.label)} in ${side}`);
  }
  if (rev.length > 0) {
    parts.push(`${rev.length} behind`);
  }
  return `${parts.join('. ')}. Follow the action banner.`;
}

export function objectDistanceLabel(camera = 'forward') {
  return camera === 'reverse' ? 'Behind' : 'Ahead';
}

export function trustPulse(objects) {
  const active = objects.filter((o) => !o.fading);
  if (active.length === 0) {
    return { pct: 100, level: 'high', label: 'Clear' };
  }

  const avg = active.reduce((sum, object) => sum + (object.confidence ?? 0), 0) / active.length;
  const pct = Math.round(avg * 100);

  if (pct >= 75) return { pct, level: 'high', label: 'Confident' };
  if (pct >= 50) return { pct, level: 'mid', label: 'Tracking' };
  return { pct, level: 'low', label: 'Uncertain' };
}
