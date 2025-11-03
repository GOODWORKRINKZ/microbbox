# Branch Protection Configuration

This document explains how to configure and use the automated branch protection workflow.

## Overview

The `branch-protection.yml` workflow automates the setup of branch protection rules for:
- `main` - Primary release branch
- `develop` - Active development branch
- `support/*` - Long-term support branches

## Important Note: Permissions Required

⚠️ **The default `GITHUB_TOKEN` cannot modify branch protection rules.**

GitHub Actions workflows use a token with limited permissions. Branch protection requires `administration` permission which is not available to the default token.

## Usage

### Option 1: Manual Configuration (Recommended)

Configure branch protection rules manually via GitHub UI:

1. Go to **Settings** → **Branches**
2. Click **Add branch protection rule**
3. Apply the following rules:

#### For `main` branch:
- Branch name pattern: `main`
- ✅ Require a pull request before merging
  - Required approvals: 1
  - Dismiss stale pull request approvals when new commits are pushed
- ✅ Require status checks to pass before merging
  - Required checks: `build-test`, `check`
  - Require branches to be up to date before merging
- ✅ Require conversation resolution before merging
- ❌ Do not allow bypassing the above settings (enforce_admins: false)
- ❌ Do not allow force pushes
- ❌ Do not allow deletions

#### For `develop` branch:
- Branch name pattern: `develop`
- ✅ Require a pull request before merging
  - Required approvals: 1
- ✅ Require status checks to pass before merging
  - Required checks: `build-test`, `check`
  - Require branches to be up to date before merging
- ❌ Do not allow force pushes
- ❌ Do not allow deletions

#### For `support/*` branches:
- Branch name pattern: `support/*`
- ✅ Require a pull request before merging
  - Required approvals: 1
  - Dismiss stale pull request approvals when new commits are pushed
- ✅ Require status checks to pass before merging
  - Required checks: `build-test`
  - Require branches to be up to date before merging
- ✅ Require conversation resolution before merging
- ❌ Do not allow force pushes
- ❌ Do not allow deletions

### Option 2: Automated Configuration with PAT

To enable automated branch protection setup:

1. **Create a Personal Access Token (PAT)**
   - Go to GitHub Settings → Developer settings → Personal access tokens → Tokens (classic)
   - Click "Generate new token (classic)"
   - Select scopes: `repo` (full control of private repositories)
   - Generate and copy the token

2. **Add token as repository secret**
   - Go to repository Settings → Secrets and variables → Actions
   - Click "New repository secret"
   - Name: `ADMIN_TOKEN`
   - Value: Paste your PAT
   - Click "Add secret"

3. **Update the workflow**
   
   Edit `.github/workflows/branch-protection.yml` and add the `github-token` parameter to each step that uses `actions/github-script@v7`:

   ```yaml
   - name: Configure Main Branch Protection
     uses: actions/github-script@v7
     with:
       github-token: ${{ secrets.ADMIN_TOKEN }}  # Add this line
       script: |
         # ... existing script
   ```

4. **Run the workflow**
   - Go to Actions → Branch Protection Setup
   - Click "Run workflow"
   - Select the branch pattern to configure
   - Click "Run workflow"

## Workflow Triggers

The workflow can be triggered:

1. **Manually** (workflow_dispatch):
   - Go to Actions → Branch Protection Setup
   - Click "Run workflow"
   - Choose branch pattern: `main`, `develop`, `support/*`, or `all`

2. **Automatically** (on push to support branches):
   - When new commits are pushed to any `support/**` branch
   - Automatically configures protection for that branch

## Troubleshooting

### Workflow fails with "Resource not accessible by integration"

This is expected when using the default `GITHUB_TOKEN`. The workflow will:
- Show warnings with manual setup instructions
- Complete successfully (not fail)
- Display a summary with configuration steps

To fix: Use Option 1 (manual) or Option 2 (PAT) above.

### Status checks not found

If you see warnings about required status checks not existing:
- Ensure the CI workflow (`.github/workflows/ci.yml`) is properly configured
- The checks `build-test` and `check` must run successfully at least once
- Status checks are case-sensitive

### Support branch pattern not working

- Ensure branch names start with `support/` (e.g., `support/v1.0`, `support/v2.x`)
- GitHub requires the branch to exist before protection can be applied
- Use wildcard pattern `support/*` in branch protection rules to cover all support branches

## Related Documentation

- [GitHub Branch Protection Rules](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches)
- [GitHub Actions Permissions](https://docs.github.com/en/actions/security-guides/automatic-token-authentication#permissions-for-the-github_token)
- [BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md) - Project branching strategy
