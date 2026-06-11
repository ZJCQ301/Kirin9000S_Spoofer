#!/system/bin/sh

MODDIR=${0%/*}

# 清理旧挂载（如果有）
umount /proc/cpuinfo 2>/dev/null

# 这个模块不需要生成 fake 文件，因为改用 hook 方式
# 但保留目录结构以兼容旧版
touch $MODDIR/cpuinfo

exit 0
