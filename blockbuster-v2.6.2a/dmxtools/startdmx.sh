#!/usr/bin/env bash
#/usr/bin/startx /usr/bin/startkde -- /usr/bin/Xdmx :2001 -norender -glxfinishswap -glxsyncswap -ignorebadfontpaths +xinerama -disablexineramaextension -fontpath unix/:7100 -configfile /usr/gapps/asciviz/blockbuster/dev/chaos_4_x86_64_ib/blockbuster/dmxtools/stagg-dmx.config -config TelepathLite -input stagg0/unix:0

/usr/bin/startx /usr/bin/startkde -- /usr/bin/Xdmx :2001 -norender -glxfinishswap -glxsyncswap -ignorebadfontpaths +xinerama -disablexineramaextension -fontpath unix/:7100 -configfile /usr/gapps/asciviz/blockbuster/dev/chaos_4_x86_64_ib/blockbuster/dmxtools/stagg-dmx.config -config TelepathLite -input $DISPLAY

