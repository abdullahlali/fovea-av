import { cabinWarnings } from './warnings.js';

function isGrokPlaceholder(text) {
  if (!text) return true;
  const t = text.replace(/^\[offline mode\]\s*/i, '').trim().toLowerCase();
  return (
    t.includes('grok unavailable') ||
    t.includes('using local passenger') ||
    (t.startsWith('{') && t.includes('"error"'))
  );
}

function summarizeObjects(objects) {
  const counts = {};
  for (const o of objects) {
    const k = (o.label ?? 'object').toLowerCase();
    counts[k] = (counts[k] ?? 0) + 1;
  }
  return counts;
}

function describeCounts(counts, where) {
  const parts = Object.entries(counts).map(([label, n]) => `${n} ${label}${n > 1 ? 's' : ''}`);
  if (parts.length === 0) return '';
  return `${parts.join(', ')} ${where}`;
}

/**
 * Action-oriented instruction for the driver (always detection-aware).
 */
export function driverInstruction(forwardObjects, reverseObjects, grokText, gear = 'D') {
  const fwd = forwardObjects.filter((o) => !o.fading);
  const rev = reverseObjects.filter((o) => !o.fading);
  const warnings = cabinWarnings(fwd, rev, gear);
  const top = warnings[0];

  if (top && top.level !== 'info') {
    const action =
      top.level === 'critical'
        ? 'Act now'
        : top.level === 'warn'
          ? 'Prepare to slow'
          : 'Stay alert';
    return {
      headline: top.text,
      detail: top.sub ?? action,
      action,
      level: top.level,
    };
  }

  if (!isGrokPlaceholder(grokText) && grokText.trim()) {
    return {
      headline: 'Copilot',
      detail: grokText.replace(/^\[offline mode\]\s*/i, '').trim(),
      action: 'Follow guidance',
      level: 'info',
    };
  }

  if (gear === 'R') {
    if (rev.length === 0) {
      return { headline: 'Clear to reverse', detail: 'Check mirrors, then back slowly.', action: 'Proceed', level: 'info' };
    }
    return {
      headline: 'Stop — obstacle behind',
      detail: describeCounts(summarizeObjects(rev), 'behind you') + '. Wait or clear path.',
      action: 'Do not reverse',
      level: 'critical',
    };
  }

  if (fwd.length === 0 && rev.length === 0) {
    return {
      headline: 'Path clear',
      detail: 'Maintain lane and normal following distance.',
      action: 'Continue',
      level: 'info',
    };
  }

  const parts = [];
  if (fwd.length > 0) parts.push(describeCounts(summarizeObjects(fwd), 'ahead'));
  if (rev.length > 0) parts.push(describeCounts(summarizeObjects(rev), 'behind'));

  return {
    headline: fwd.some((o) => (o.label ?? '').toLowerCase().includes('person'))
      ? 'Watch for pedestrians'
      : 'Traffic nearby',
    detail: parts.join(' · ') + '. Adjust speed and keep safe gap.',
    action: fwd.length > 0 ? 'Ease off accelerator' : 'Monitor blind spots',
    level: fwd.length > 2 ? 'warn' : 'info',
  };
}

export function passengerBriefing(forwardObjects, reverseObjects, grokText, gear = 'D') {
  return driverInstruction(forwardObjects, reverseObjects, grokText, gear).detail;
}
