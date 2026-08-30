# Grok API setup (cost-conscious)

## xAI console — recommended limits for `fovea-av`

| Field | Recommendation | Why |
|-------|----------------|-----|
| **Name** | `fovea-av` | Good |
| **Models** | `grok-4.20-0309-non-reasoning` only | Cheapest fit for short narration (no reasoning tokens) |
| **Endpoints** | Chat completions only (if available) | Fovea only uses `/chat/completions` |
| **Requests per minute** | `10`–`20` | Plenty for dev; stops runaway loops |
| **Tokens per minute** | `20,000`–`50,000` | Caps burst spend |
| **Expiry** | 90 days (optional) | Rotate keys for portfolio projects |

Also set a **prepaid credit cap** in the xAI billing console if available.

## Local setup (never commit the key)

```bash
# In project root — already gitignored
echo 'XAI_API_KEY=xai-...' > .env
export $(grep -v '^#' .env | xargs)
```

Or add to `~/.zshrc` (less ideal — global):

```bash
export XAI_API_KEY="xai-..."
```

## What Fovea already does to save cost

- Sends **scene graph JSON only** (not the full image) — small input
- **`max_tokens: 120`** — short passenger-facing answers
- **`grok-4.20-0309-non-reasoning`** default — cheap, no reasoning overhead for 2–3 sentence answers
- **`temperature: 0.2`** — consistent, less rambling
- Grok only runs with **`--grok`** flag — normal runs are free

## Rough cost mental model

One narration ≈ a few hundred input tokens + ≤120 output tokens.  
At dev volume (dozens of runs), expect **cents not dollars** if limits are set.

## If you hit limits

Console returns HTTP 429 — Fovea will show the error in CLI/Qt. Wait a minute or raise RPM slightly.
