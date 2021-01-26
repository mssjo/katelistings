#!/bin/bash

echo "Enabling scripts (may require sudo)"
chmod +x scripts/get_data.sh && chmod +x scripts/map_languages.sh || exit 1

echo "Compiling program"
cmake . && make katelistings && make map_languages || exit 1

echo "Obtaining data"
katelistings --get-data echo "Installation successful."
