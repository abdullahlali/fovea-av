/** @typedef {{ label: string, confidence: number, fading?: boolean, id?: number, camera?: string }} TrackedObject */

/**
 * @param {Array<TrackedObject>} objects
 * @param {'forward' | 'reverse'} camera
 * @param {string} gear
 * @returns {Array<{ level: 'critical' | 'warn' | 'info', text: string, sub?: string, camera: string }>}
 */
export function roadWarnings(objects, camera, gear = 'D') {
  const active = objects.filter((o) => !o.fading);
  const warnings = [];

  const people = active.filter((o) => isPerson(o.label));
  const vehicles = active.filter((o) => isVehicle(o.label));
  const trucks = active.filter((o) => isTruck(o.label));

  if (camera === 'forward') {
    if (people.length > 0) {
      warnings.push({
        level: 'critical',
        text: 'PEDESTRIAN AHEAD',
        sub: 'Reduce speed immediately',
        camera,
      });
    }
    if (trucks.length > 0) {
      warnings.push({
        level: 'warn',
        text: 'LARGE VEHICLE AHEAD',
        sub: 'Increase following distance',
        camera,
      });
    } else if (vehicles.length > 0) {
      warnings.push({
        level: 'warn',
        text: 'SLOW DOWN',
        sub: 'Traffic detected ahead',
        camera,
      });
    }
    if (active.length >= 3) {
      warnings.push({
        level: 'info',
        text: 'BUSY ROAD',
        sub: 'Stay alert',
        camera,
      });
    }
  }

  if (camera === 'reverse') {
    if (gear === 'R' || gear === 'D') {
      if (people.length > 0) {
        warnings.push({
          level: 'critical',
          text: 'PERSON BEHIND',
          sub: 'Stop and check surroundings',
          camera,
        });
      }
      if (vehicles.length > 0) {
        warnings.push({
          level: 'warn',
          text: 'VEHICLE BEHIND',
          sub: 'Check blind spots',
          camera,
        });
      }
      if (gear === 'R' && active.length === 0) {
        warnings.push({
          level: 'info',
          text: 'CLEAR TO REVERSE',
          sub: 'Proceed with caution',
          camera,
        });
      }
    }
  }

  if (warnings.length === 0 && active.length === 0) {
    warnings.push({
      level: 'info',
      text: camera === 'forward' ? 'ROAD CLEAR' : 'AREA CLEAR',
      sub: 'No hazards detected',
      camera,
    });
  }

  return warnings;
}

/**
 * @param {Array<TrackedObject>} forwardObjects
 * @param {Array<TrackedObject>} reverseObjects
 * @param {string} gear
 */
export function cabinWarnings(forwardObjects, reverseObjects, gear) {
  const showForward = gear === 'D' || gear === 'N' || gear === 'P';
  const showReverse = gear === 'D' || gear === 'R' || gear === 'N';

  const warnings = [];
  if (showForward) {
    warnings.push(...roadWarnings(forwardObjects, 'forward', gear));
  }
  if (showReverse) {
    warnings.push(...roadWarnings(reverseObjects, 'reverse', gear));
  }

  const priority = { critical: 0, warn: 1, info: 2 };
  return warnings.sort((a, b) => priority[a.level] - priority[b.level]);
}

function isPerson(label = '') {
  const k = label.toLowerCase();
  return k.includes('person') || k.includes('pedestrian');
}

function isVehicle(label = '') {
  const k = label.toLowerCase();
  return k.includes('car') || k.includes('vehicle') || k.includes('bus') || k.includes('truck');
}

function isTruck(label = '') {
  const k = label.toLowerCase();
  return k.includes('truck') || k.includes('bus');
}

export function gearLabel(gear) {
  switch (gear) {
    case 'P':
      return 'Park';
    case 'R':
      return 'Reverse';
    case 'N':
      return 'Neutral';
    case 'D':
    default:
      return 'Drive';
  }
}

export function visibleCamerasForGear(gear, focus) {
  if (focus === 'forward') return ['forward'];
  if (focus === 'reverse') return ['reverse'];
  if (gear === 'R') return ['reverse'];
  if (gear === 'P') return ['forward', 'reverse'];
  return ['forward', 'reverse'];
}
