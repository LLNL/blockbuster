#!/usr/bin/env bash

pdsh -w estagg[1-4] /usr/bin/xinit /bin/sleep 1d -- /usr/bin/X -r -kb :0 -auth /g/g0/rcook/.Xauthority.stagg -config xorg.conf
