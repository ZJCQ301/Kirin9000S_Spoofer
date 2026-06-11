#!/system/bin/sh

MODDIR=${0%/*}
FAKE_CPUINFO="$MODDIR/cpuinfo"
FAKE_MIDR="$MODDIR/midr_el1"
FAKE_VERSION="$MODDIR/version"
FAKE_MOUNTINFO="$MODDIR/mountinfo"
FAKE_SOCID="$MODDIR/soc_id"

# 生成随机 BogoMIPS 值（26~30 之间浮动，每次安装不同）
BOGO_BASE=$((26 + RANDOM % 5))
BOGO_DEC=$((RANDOM % 99))
BOGO_MIPS="$BOGO_BASE.$BOGO_DEC"

# 写入伪造的 /proc/cpuinfo（动态 BogoMIPS）
cat > $FAKE_CPUINFO <<EOF
Processor	: AArch64 Processor rev 0 (aarch64)
Features	: fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x1
CPU part	: 0xd0c
CPU revision	: 0

processor	: 0
BogoMIPS	: $BOGO_MIPS
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x1
CPU part	: 0xd0c
CPU revision	: 0

processor	: 1
BogoMIPS	: $BOGO_MIPS
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x2
CPU part	: 0xd0a
CPU revision	: 0

processor	: 2
BogoMIPS	: $BOGO_MIPS
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x2
CPU part	: 0xd0a
CPU revision	: 0

processor	: 3
BogoMIPS	: $BOGO_MIPS
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x2
CPU part	: 0xd0a
CPU revision	: 0

processor	: 4
BogoMIPS	: $BOGO_MIPS
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x3
CPU part	: 0xd0b
CPU revision	: 0

processor	: 5
BogoMIPS	: $BOGO_MIPS
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x3
CPU part	: 0xd0b
CPU revision	: 0

processor	: 6
BogoMIPS	: $BOGO_MIPS
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x3
CPU part	: 0xd0b
CPU revision	: 0

processor	: 7
BogoMIPS	: $BOGO_MIPS
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x3
CPU part	: 0xd0b
CPU revision	: 0

Hardware	: HiSilicon Kirin 9000S
EOF

# midr_el1
echo "0x480fd0c0" > $FAKE_MIDR

# 伪造的内核版本（模拟麒麟9000S常见内核）
cat > $FAKE_VERSION <<EOF
Linux version 5.10.43-android12-9-g12345678-abcd1234 (build-user@build-host) (clang version 14.0.7, lld 14.0.7) #1 SMP PREEMPT Thu Jun 12 10:00:00 CST 2026
EOF

# 伪造的 mountinfo（只保留基础挂载，隐藏 bind mount）
cat > $FAKE_MOUNTINFO <<EOF
20 0 0:1 / /proc rw,relatime - proc proc rw
21 0 0:2 / /dev rw,relatime - tmpfs tmpfs rw
22 0 0:3 / /sys rw,relatime - sysfs sysfs rw
23 0 0:4 / /data rw,relatime - f2fs /dev/block/sda23 rw,seclabel
EOF

# 伪造的 soc_id
echo "Kirin9000S" > $FAKE_SOCID

# 设置权限
chmod 644 $FAKE_CPUINFO $FAKE_MIDR $FAKE_VERSION $FAKE_MOUNTINFO $FAKE_SOCID

# 清理可能残留的旧挂载
umount /proc/cpuinfo 2>/dev/null
umount /proc/version 2>/dev/null
umount /proc/self/mountinfo 2>/dev/null
umount /sys/devices/soc0/soc_id 2>/dev/null

exit 0
