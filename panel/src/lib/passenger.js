/** Hold detections briefly so passenger UI does not flicker frame-to-frame. */
const HOLD_MS = 1800;
const CONFIDENCE_BLEND = 0.35;

/**
 * @param {Map<number, object>} stable
 * @param {Array<object>} incoming
 * @returns {Map<number, object>}
 */
export function mergeStableTracks(stable, incoming) {
  const now = Date.now();
  const next = new Map(stable);

  for (const detection of incoming) {
    const existing = next.get(detection.id);
    const blendedConfidence = existing
      ? existing.confidence * (1 - CONFIDENCE_BLEND) + detection.confidence * CONFIDENCE_BLEND
      : detection.confidence;

    next.set(detection.id, {
      ...detection,
      confidence: blendedConfidence,
      lastSeen: now,
      fading: false,
    });
  }

  for (const [id, track] of next) {
    if (now - track.lastSeen > HOLD_MS) {
      next.delete(id);
    } else if (!incoming.some((d) => d.id === id)) {
      next.set(id, { ...track, fading: true });
    }
  }

  return next;
}

export function normalizeGrokText(text) {
  if (!text) return '';
  return text.replace(/^\[offline mode\]\s*/i, '').trim();
}

/**
 * @param {Array<{label: string, confidence: number, fading?: boolean}>} objects
 */
export function passengerBriefing(objects, grokText) {
  const narration = normalizeGrokText(grokText);
  if (narration) return narration;
  if (objects.length === 0) {
    return 'The road ahead looks clear. No vehicles or pedestrians detected nearby.';
  }

  const active = objects.filter((o) => !o.fading);
  const list = (active.length > 0 ? active : objects).map((o) => ({
    label: o.label.toLowerCase(),
    pct: Math.round(o.confidence * 100),
  }));

  if (list.length === 1) {
    const item = list[0];
    return `There is a ${item.label} in the forward view. The vehicle is ${confidenceWord(item.pct)} it is there.`;
  }

  const head = list
    .slice(0, -1)
    .map((o) => `a ${o.label}`)
    .join(', ');
  const tail = list[list.length - 1];
  return `The vehicle sees ${head}, and a ${tail.label} ahead.`;
}

function confidenceWord(pct) {
  if (pct >= 80) return 'confident';
  if (pct >= 55) return 'fairly confident';
  return 'cautiously tracking';
}

export function objectDistanceLabel() {
  return 'In forward view';
}
