#!/bin/bash

VERSION_BUILD_FILE="include/kernel/version_build.h"
BUILD_NUMBER_FILE=".build_number"

GIT_HASH=$(git rev-parse --short=7 HEAD 2>/dev/null || echo "dev-build")

BUILD_DATE=$(date +%d/%m/%Y)
BUILD_TIME=$(date +%H:%M:%S)

if [ -f "$BUILD_NUMBER_FILE" ]; then
    BUILD_NUM=$(cat "$BUILD_NUMBER_FILE")
else
    BUILD_NUM=0
fi
BUILD_NUM=$((BUILD_NUM + 1))
echo $BUILD_NUM > "$BUILD_NUMBER_FILE"

cat > "$VERSION_BUILD_FILE" << EOF
#ifndef VERSION_BUILD_H
#define VERSION_BUILD_H

#define OS_BUILD_HASH "$GIT_HASH"
#define OS_BUILD_DATE "$BUILD_DATE"
#define OS_BUILD_TIME "$BUILD_TIME"
#define OS_BUILD_NUMBER "$BUILD_NUM"

#endif // VERSION_BUILD_H
EOF

echo "Build info generated:"
echo "  Hash: $GIT_HASH"
echo "  Date: $BUILD_DATE $BUILD_TIME"
echo "  Build #$BUILD_NUM"
