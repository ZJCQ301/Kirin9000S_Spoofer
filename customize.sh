#!/system/bin/sh

MODDIR=${0%/*}
FAKE_CPUINFO="$MODDIR/cpuinfo"
FAKE_MIDR="$MODDIR/midr_el1"
FAKE_VERSION="$MODDIR/version"
FAKE_MOUNTINFO="$MODDIR/mountinfo"
FAKE_SOCID="$MODDIR/soc_id"
FAKE_STAT="$MODDIR/stat"

BOGO_BASE=$((26 + RANDOM % 5))
BOGO_DEC=$((RANDOM % 99))
BOGO_MIPS="$BOGO_BASE.$BOGO_DEC"

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

echo "0x480fd0c0" > $FAKE_MIDR

cat > $FAKE_VERSION <<EOF
Linux version 5.10.43-android12-9-g12345678-abcd1234 (build-user@build-host) (clang version 14.0.7, lld 14.0.7) #1 SMP PREEMPT Thu Jun 12 10:00:00 CST 2026
EOF

cat > $FAKE_MOUNTINFO <<EOF
20 0 0:1 / /proc rw,relatime - proc proc rw
21 0 0:2 / /dev rw,relatime - tmpfs tmpfs rw
22 0 0:3 / /sys rw,relatime - sysfs sysfs rw
23 0 0:4 / /data rw,relatime - f2fs /dev/block/sda23 rw,seclabel
EOF

echo "Kirin9000S" > $FAKE_SOCID

cat > $FAKE_STAT <<EOF
cpu  88786 0 25674 892056 3614 0 874 0 0 0
cpu0 12345 0 3456 123456 567 0 123 0 0 0
cpu1 12345 0 3456 123456 567 0 123 0 0 0
cpu2 12345 0 3456 123456 567 0 123 0 0 0
cpu3 12345 0 3456 123456 567 0 123 0 0 0
cpu4 12345 0 3456 123456 567 0 123 0 0 0
cpu5 12345 0 3456 123456 567 0 123 0 0 0
cpu6 12345 0 3456 123456 567 0 123 0 0 0
cpu7 12345 0 3456 123456 567 0 123 0 0 0
intr 1023456 0 0 0 0 0 0 0 0 0
ctxt 9876543
btime 1600000000
processes 123456
procs_running 2
procs_blocked 0
softirq 654321 0 0 0 0 0 0 0 0 0
EOF

echo "0-7" > $MODDIR/kernel_max
echo "0-7" > $MODDIR/possible
echo "0-7" > $MODDIR/present
echo "8" > $MODDIR/cpu_present
echo "8" > $MODDIR/cpu_possible

for i in 0 1 2 3 4 5 6 7; do
    mkdir -p $MODDIR/cpu$i
    echo "1" > $MODDIR/cpu$i/online
done

chmod 644 $FAKE_CPUINFO $FAKE_MIDR $FAKE_VERSION $FAKE_MOUNTINFO $FAKE_SOCID $FAKE_STAT
chmod 644 $MODDIR/kernel_max $MODDIR/possible $MODDIR/present $MODDIR/cpu_present $MODDIR/cpu_possible
for i in 0 1 2 3 4 5 6 7; do
    chmod 644 $MODDIR/cpu$i/online
done

umount /proc/cpuinfo 2>/dev/null
umount /proc/version 2>/dev/null
umount /proc/self/mountinfo 2>/dev/null
umount /sys/devices/soc0/soc_id 2>/dev/null
umount /proc/stat 2>/dev/null
umount /sys/devices/system/cpu/kernel_max 2>/dev/null
umount /sys/devices/system/cpu/possible 2>/dev/null
umount /sys/devices/system/cpu/present 2>/dev/null
for i in 0 1 2 3 4 5 6 7; do
    umount /sys/devices/system/cpu/cpu$i/online 2>/dev/null
done

exit 0
