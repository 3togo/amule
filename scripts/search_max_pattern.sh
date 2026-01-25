#!/bin/bash

# Search for max( pattern in feature/bitTorrent-integration branch
# Usage: ./search_max_pattern.sh

# Clone the repository if not already present
if [ ! -d "amule" ]; then
    git clone https://github.com/amule-project/amule.git
fi

cd amule

# Checkout the feature branch
git checkout feature/bitTorrent-integration 2>/dev/null || git fetch origin feature/bitTorrent-integration && git checkout feature/bitTorrent-integration

# Search for std::max( pattern with context
echo "Searching for std::max( pattern in feature/bitTorrent-integration branch..."
echo "--------------------------------------------------"

grep --color=always -A2 -B2 -r -E "std::max\(" src/

echo "--------------------------------------------------"
echo "Search complete. Results shown above."