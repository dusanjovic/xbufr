#!/bin/bash
set -eu

[ "$(uname -s)" != "Darwin" ] && echo "Run this script on macOS" && exit 2;

INSTALL_DIR=$1
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

title=xbufr
size=150000
dmg_name=xbufr.dmg
TMP=db_tmp

####################################################################################

rm -rf ${TMP}
mkdir -p ${TMP}

cp "${INSTALL_DIR}"/bin/bufr_tables.db "${INSTALL_DIR}"/bin/xbufr.app/Contents/MacOS
cp -r "${INSTALL_DIR}"/bin/xbufr.app ${TMP}

rm -f pack.temp.dmg
hdiutil create -srcfolder ${TMP} -volname "${title}" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${size}k pack.temp.dmg

rm -rf ${TMP}

MOUNT_DIR="/Volumes/${title}"

if [[ -d "${MOUNT_DIR}" ]]; then
    device=$(hdiutil info | grep "${MOUNT_DIR}" | sed 1q | awk '{print $1}')
    hdiutil detach "${device}"
fi

device=$(hdiutil attach -readwrite -noverify -noautoopen pack.temp.dmg | grep "${title}" | sed 1q | awk '{print $1}')

cp "${MYDIR}"/images/xbufr_dmg.icns "${MOUNT_DIR}"/.VolumeIcon.icns
SetFile -c icnC "${MOUNT_DIR}"/.VolumeIcon.icns
SetFile -a C "${MOUNT_DIR}"

bless --folder "${MOUNT_DIR}" --openfolder "${MOUNT_DIR}"

echo '
   tell application "Finder"
     tell disk "'${title}'"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {100, 100, 600, 400}
           set theViewOptions to the icon view options of container window
           set arrangement of theViewOptions to not arranged
           set icon size of theViewOptions to 72
           make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
           set position of item "xbufr.app" of container window to {125, 120}
           set position of item "Applications" of container window to {375, 120}
           close
     end tell
   end tell
' | osascript

chmod -Rf go-w "${MOUNT_DIR}"
sync
sync
hdiutil detach ${device}

rm -f ${dmg_name}
hdiutil convert pack.temp.dmg -format UDZO -imagekey zlib-level=9 -o "${dmg_name}"
rm -f pack.temp.dmg
