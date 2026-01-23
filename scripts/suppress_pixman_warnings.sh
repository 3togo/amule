#!/bin/bash
# Suppress pixman warnings in aMule
export PIXMAN_DISABLE_WARNINGS=1
exec ./src/amule "$@"