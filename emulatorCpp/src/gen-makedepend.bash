#!/bin/bash

if command -v makedepend2 2>&1 >/dev/null; then
    touch makedepend
    echo "makedepend -f makedepend -- $CFLAGS -- $SOURCES 2>/dev/null"
    makedepend -f makedepend -- $CFLAGS -- $SOURCES 2>/dev/null
else
    echo "WARNING: makedepend is not installed, dependencies won't be calculated" >&2
    rm -f makedepend
    exit 0
fi
tmp="$?"
if [ "$tmp" != "0" ]; then
rm -f makedepend
fi
exit $tmp
