#!/bin/bash

# Script to calculate the next version based on release type
# Usage: ./calculate_next_version.sh [major|minor|patch]

set -e

RELEASE_TYPE=$1

if [ -z "$RELEASE_TYPE" ]; then
  echo "Error: Release type not specified"
  echo "Usage: $0 [major|minor|patch]"
  exit 1
fi

if [[ ! "$RELEASE_TYPE" =~ ^(major|minor|patch)$ ]]; then
  echo "Error: Invalid release type. Must be 'major', 'minor', or 'patch'"
  exit 1
fi

# Default initial version if no tags exist
DEFAULT_VERSION="v0.0.0"

# Get the latest tag that matches v*.*.* pattern
LATEST_TAG=$(git describe --tags --abbrev=0 --match='v*.*.*' 2>/dev/null || echo "$DEFAULT_VERSION")

echo "Latest tag: $LATEST_TAG" >&2

# Remove 'v' prefix and split version into components
VERSION_WITHOUT_V=${LATEST_TAG#v}
IFS='.' read -r MAJOR MINOR PATCH <<< "$VERSION_WITHOUT_V"

# Validate semantic versioning format and default to 0 if components are empty or invalid
if ! [[ "$MAJOR" =~ ^[0-9]+$ ]]; then MAJOR=0; fi
if ! [[ "$MINOR" =~ ^[0-9]+$ ]]; then MINOR=0; fi
if ! [[ "$PATCH" =~ ^[0-9]+$ ]]; then PATCH=0; fi

echo "Current version: $MAJOR.$MINOR.$PATCH" >&2

# Calculate next version based on release type
case $RELEASE_TYPE in
  major)
    MAJOR=$((MAJOR + 1))
    MINOR=0
    PATCH=0
    ;;
  minor)
    MINOR=$((MINOR + 1))
    PATCH=0
    ;;
  patch)
    PATCH=$((PATCH + 1))
    ;;
esac

NEXT_VERSION="v$MAJOR.$MINOR.$PATCH"
echo "Next version: $NEXT_VERSION" >&2

# Output only the version (for use in GitHub Actions)
echo "$NEXT_VERSION"
