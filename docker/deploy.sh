#!/bin/bash
# Run this from the repo root.

set -euo pipefail

tag="mvcorreia/miplib-backends:1.3.0"

# Build and tag image
docker build --pull -t $tag -f docker/Dockerfile .
docker push $tag

