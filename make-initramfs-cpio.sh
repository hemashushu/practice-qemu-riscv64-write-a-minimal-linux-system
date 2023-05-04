#!/bin/bash
set -ex

pushd apps
riscv64-linux-gnu-gcc -g -Wall -static -o init init.c
riscv64-linux-gnu-gcc -g -Wall -static -o mount mount.c
riscv64-linux-gnu-gcc -g -Wall -static -o umount umount.c
riscv64-linux-gnu-gcc -g -Wall -static -o poweroff poweroff.c
riscv64-linux-gnu-gcc -g -Wall -static -o sh sh.c
riscv64-linux-gnu-gcc -g -Wall -static -o echo echo.c
riscv64-linux-gnu-gcc -g -Wall -static -o pwd pwd.c
riscv64-linux-gnu-gcc -g -Wall -static -o cat cat.c
riscv64-linux-gnu-gcc -g -Wall -static -o ls ls.c
riscv64-linux-gnu-gcc -g -Wall -static -o time time.c
riscv64-linux-gnu-gcc -g -Wall -static -o applets applets.c
popd

mkdir -p initramfs

pushd initramfs

mkdir -p bin
mkdir -p sbin
mkdir -p usr/bin
mkdir -p etc
mkdir -p root
mkdir -p proc
mkdir -p sys

# optional
mkdir -p run
mkdir -p tmp
mkdir -p dev

echo "Hello, My own Linux system!" > root/hello.txt

cat << "EOF" > etc/rc
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys

# optional
mount -t tmpfs tmpfs /run
mount -t tmpfs tmpfs /tmp
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
cp ../../apps/poweroff poweroff
popd

pushd bin
cp ../../apps/sh sh
cp ../../apps/echo echo
cp ../../apps/pwd pwd
cp ../../apps/ls ls
cp ../../apps/cat cat
popd

pushd usr/bin
cp ../../../apps/time time
cp ../../../apps/applets applets
test -L tee || ln -s applets tee
test -L tr || ln -s applets tr
test -L uname || ln -s applets uname
test -L poweroff || ln -s applets poweroff
popd

find . | \
     cpio -o -v --format=newc | \
     gzip > ../initramfs.cpio.gz
popd