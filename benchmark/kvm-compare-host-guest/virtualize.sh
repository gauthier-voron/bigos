#!/bin/sh

APPEND="root=/dev/sda3 console=ttyS0"
MEMORY=200G

VERSION="3.9.7"
KERNEL="/boot/vmlinuz-$VERSION"
INITRD="/boot/initrd.img-$VERSION"


qemu-system-x86_64 -snapshot -hda /dev/sda \
  -m "$MEMORY" -kernel "$KERNEL" -initrd "$INITRD" -append "$APPEND" \
  -enable-kvm -net nic -net user,net=132.227.105.0/24,host=132.227.105.254,hostfwd=tcp::2222-:22 \
  -cpu host -smp 48 \
  -numa node,nodeid=0,cpus=0-5 \
  -numa node,nodeid=1,cpus=6-11 \
  -numa node,nodeid=2,cpus=36-41 \
  -numa node,nodeid=3,cpus=42-47 \
  -numa node,nodeid=4,cpus=24-29 \
  -numa node,nodeid=5,cpus=30-35 \
  -numa node,nodeid=6,cpus=12-17 \
  -numa node,nodeid=7,cpus=18-23
