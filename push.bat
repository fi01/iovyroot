
adb push "%cd%\libs\arm64-v8a\iovyroot" /data/local/tmp/iovyroot
adb shell chmod 755 /data/local/tmp/iovyroot

adb push "%cd%\supolicy" /data/local/tmp/supolicy
adb shell chmod 755 /data/local/tmp/supolicy

adb push "%cd%\libsupol.so" /data/local/tmp/libsupol.so
adb shell chmod 755 /data/local/tmp/libsupol.so

adb push "%cd%\patch_script.sh" /data/local/tmp/patch_script.sh
adb shell chmod 755 /data/local/tmp/patch_script.sh

adb push "%cd%\busybox-armv7l" /data/local/tmp/busybox
adb shell chmod 755 /data/local/tmp/busybox
