#!/system/bin/sh

MODDIR=${0%/*}
FAKE_CPUINFO="$MODDIR/cpuinfo"
FAKE_MIDR="$MODDIR/midr_el1"

# 写入伪造的 /proc/cpuinfo
cat > $FAKE_CPUINFO <<EOF
Processor	: AArch64 Processor rev 0 (aarch64)
Features	: fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x1
CPU part	: 0xd0c
CPU revision	: 0

processor	: 0
BogoMIPS	: 26.00
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x1
CPU part	: 0xd0c
CPU revision	: 0

processor	: 1
BogoMIPS	: 26.00
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x2
CPU part	: 0xd0a
CPU revision	: 0

processor	: 2
BogoMIPS	: 26.00
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x2
CPU part	: 0xd0a
CPU revision	: 0

processor	: 3
BogoMIPS	: 26.00
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x2
CPU part	: 0xd0a
CPU revision	: 0

processor	: 4
BogoMIPS	: 26.00
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x3
CPU part	: 0xd0b
CPU revision	: 0

processor	: 5
BogoMIPS	: 26.00
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x3
CPU part	: 0xd0b
CPU revision	: 0

processor	: 6
BogoMIPS	: 26.00
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x3
CPU part	: 0xd0b
CPU revision	: 0

processor	: 7
BogoMIPS	: 26.00
CPU implementer	: 0x48
CPU architecture: 8
CPU variant	: 0x3
CPU part	: 0xd0b
CPU revision	: 0

Hardware	: HiSilicon Kirin 9000S
EOF

echo "0x480fd0c0" > $FAKE_MIDR

chmod 644 $FAKE_CPUINFO $FAKE_MIDR

# 清理旧挂载点，防止残留冲突
umount /proc/cpuinfo 2>/dev/null
umount /sys/devices/system/cpu/cpu0/regs/identification/midr_el1 2>/dev/null

exit 0