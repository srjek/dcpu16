#!/bin/bash

touch makedepend
echo "makedepend -f makedepend -- $CFLAGS -- $SOURCES 2>/dev/null"
makedepend -f makedepend -- $CFLAGS -- $SOURCES 2>/dev/null
tmp="$?"
if [ "$tmp" != "0" ]; then
rm -f makedepend
fi
exit $tmp
