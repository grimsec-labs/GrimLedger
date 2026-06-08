# GitHub migration: miditool → grimsec-labs

## Quick migrate

1. Authenticate as **grimsec-labs**:
   ```powershell
   gh auth login
   ```

2. Run from repo root (local remote setup only, no push):
   ```powershell
   .\scripts\migrate-to-grimsec-labs.ps1 -LocalOnly -GrimsecEmail "you@example.com"
   ```

3. Create the empty repo on GitHub as **grimsec-labs**, then push:
   ```powershell
   .\scripts\migrate-to-grimsec-labs.ps1 -GrimsecEmail "you@example.com"
   ```

   SSH (with `github-grimsec` host alias in `~/.ssh/config`):
   ```powershell
   .\scripts\migrate-to-grimsec-labs.ps1 -GrimsecEmail "you@example.com" -UseSsh
   ```

3. Archive the old repo (as **miditool**):
   ```powershell
   gh auth switch
   gh repo edit miditool/GrimLedger --description "Moved to https://github.com/grimsec-labs/GrimLedger"
   gh repo archive miditool/GrimLedger --yes
   ```

## SSH config for two GitHub accounts

```sshconfig
Host github.com
  HostName github.com
  User git
  IdentityFile ~/.ssh/id_ed25519
  IdentitiesOnly yes

Host github-grimsec
  HostName github.com
  User git
  IdentityFile ~/.ssh/id_ed25519_grimsec
  IdentitiesOnly yes
```

## What is not migrated

Issues, PRs, and stars stay on miditool unless you use GitHub **Transfer repository** instead of a fresh push.
