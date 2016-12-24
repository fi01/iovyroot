echo off
for /f %%i in ('adb shell getprop ro.product.cpu.abi') do (set abi=%%i)
echo on
adb push %cd%\libs\%abi%\iovyroot /data/local/tmp/iovyroot
adb shell chmod 755 /data/local/tmp/iovyroot
