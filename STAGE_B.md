# Stage B — Distinctive Workspace Features

Stage B implements the twelve concepts from `codexNewFeatures.md` as practical MVPs integrated into GrimLedger.

## Features

| # | Feature | Location | Usage |
|---|---------|----------|-------|
| 1 | **Sealed blocks** | `SealedBlockRepository`, note save | Author with `:::sealed Label` / content / `:::` — stored encrypted, rendered as placeholders |
| 2 | **Runbook sessions** | `RunbookParser`, `RunbookSessionRepository` | Settings → Start Runbook Session on checklist notes |
| 3 | **Web clipper** | Bridge `clip` action, extension popup | Enable clipper in Settings; select text → Clip selection |
| 4 | **Casebook provenance** | `AttachmentRepository` SHA-256 at import | Attachment hashes stored in `sha256_hex` column |
| 5 | **Knowledge graph** | `KnowledgeGraph` | Settings → export DOT graph of links/tags |
| 6 | **Secret scanner** | `SecretScanner` | Settings → Scan Notes for Secrets |
| 7 | **Redaction studio** | `RedactionStudio` | Settings → Redacted Export (current note) |
| 8 | **Diagram blocks** | `DiagramRenderer` | ` ```mermaid ` fenced blocks in preview |
| 9 | **GrimShare** | `GrimShare` | Settings → export/import `.grimshare` packages |
| 10 | **Semantic search** | `SemanticSearch` | Settings → enable synonym-expanded local search |
| 11 | **Crypt chambers** | `ChamberManager` | Settings → Lock/Unlock Work chamber (session re-auth) |
| 12 | **Grim Chronicle** | `SecurityChronicle` | Settings → view hash-chained security event log |

## Settings panel

Open **Vault Settings** → **Workspace Tools (Stage B)** for toggles and actions.

## Security notes

- Sealed blocks reduce accidental exposure; they do not protect against a compromised unlocked process.
- Redaction and secret scanning are heuristic — not proof of safety.
- GrimShare packages use Argon2id + AEAD (`GRIMSHR1` format).
- Chronicle hash chain detects tampering within the log, not full vault rollback.
- Crypt chambers MVP uses derived chamber keys and session lock; full per-chamber encryption migration is future work.

## Bridge clipper

Requires browser bridge **and** separate **web clipper** toggle. Desktop always confirms before saving clipped content.

## Tests

`test_stage_b` covers secret scanner, runbook parsing, and GrimShare round-trip.
