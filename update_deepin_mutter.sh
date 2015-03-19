#!/bin/bash

# migrate code from mutter to deepin-mutter
appname="$(basename $0)"

grep_ignore_files="${appname}\|README\|NEWS\|Makefile.am\|\.git\|\./po\|\./debian\|./src/core/monitor.c\|./src/core/meta-idle-monitor.c\|xrandr.xml\|idle-monitor.xml"

echo "==> show gsettings path with prefix 'org.gnome'"
find . -type f | grep -v "${grep_ignore_files}" | xargs grep 'org.gnome.mutter'

echo "==> replace gsettings path"
for f in $(find . -type f | grep -v "${grep_ignore_files}" | xargs grep -l 'org.gnome.mutter'); do
  echo "  -> ${f}"
  sed -e 's=org.gnome.mutter=com.deepin.dde.wm.mutter=' \
      -e 's=/org/gnome/mutter=/com/deepin/dde/wm/mutter=' -i "${f}"
done


