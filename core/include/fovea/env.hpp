#pragma once

namespace fovea {

// Load KEY=VALUE pairs from .env into the process environment.
// When override_existing is true, values from the file replace already-set vars.
void load_dotenv(const char* path, bool override_existing = false);

// Try common locations (cwd, parents, build output relative paths).
void load_project_dotenv();

}  // namespace fovea
