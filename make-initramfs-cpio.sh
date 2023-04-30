#!/bin/bash
set -ex

pushd apps
riscv64-linux-gnu-gcc -g -Wall -static -o init init.c
riscv64-linux-gnu-gcc -g -Wall -static -o sh sh.c
riscv64-linux-gnu-gcc -g -Wall -static -o mount mount.c
riscv64-linux-gnu-gcc -g -Wall -static -o umount umount.c
riscv64-linux-gnu-gcc -g -Wall -static -o pwd pwd.c
riscv64-linux-gnu-gcc -g -Wall -static -o cat cat.c
riscv64-linux-gnu-gcc -g -Wall -static -o ls ls.c
riscv64-linux-gnu-gcc -g -Wall -static -o applets applets.c
riscv64-linux-gnu-gcc -g -Wall -static -o fork-test fork-test.c
popd

mkdir -p initramfs

pushd initramfs

mkdir -p bin
mkdir -p sbin
mkdir -p usr/bin
mkdir -p etc
mkdir -p root
mkdir -p proc

# optional
mkdir -p sys
mkdir -p run
mkdir -p dev
mkdir -p opt

echo "Hello, My own Linux system!" > root/hello.txt

cat << "EOF" > etc/rc
#!/bin/sh
mount -t proc proc /proc

# optional
mount -t sysfs sysfs /sys
mount -t tmpfs tmpfs /run
mount -t devtmpfs devtmpfs /dev

# start an interactive shell
cd /root
/bin/sh &
EOF

chmod +x etc/rc

pushd sbin
cp ../../apps/init init
cp ../../apps/mount mount
cp ../../apps/umount umount
popd

pushd bin
cp ../../apps/sh sh
cp ../../apps/pwd pwd
cp ../../apps/ls ls
cp ../../apps/cat cat
popd

pushd usr/bin
cp ../../../apps/applets applets
test -L tee || ln -s applets tee
test -L tr || ln -s applets tr
test -L uname || ln -s applets uname
test -L poweroff || ln -s applets poweroff
popd

pushd opt
cp ../../apps/fork-test fork-test
popd

find . | \
     cpio -o -v --format=newc | \
     gzip > ../initramfs.cpio.gz
popd