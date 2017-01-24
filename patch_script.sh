#!/system/bin/sh
export LD_LIBRARY_PATH=/data/local/tmp:$LD_LIBRARY_PATH
rm /data/local/tmp/sepolicy
rm /data/local/tmp/patched_sepolicy
cp /sepolicy /data/local/tmp/sepolicy
/data/local/tmp/supolicy --file /data/local/tmp/sepolicy /data/local/tmp/patched_sepolicy "permissive zygote;permissive kernel;permissive init;permissive su;permissive init_shell;permissive shell;permissive servicemanager;"
