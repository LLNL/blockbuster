#!/usr/bin/env bash
. $(dirname $0)/shellfuncs.sh

idrunecho sedfiles --nobackups "$@"

