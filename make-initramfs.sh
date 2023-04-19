#!/bin/bash
set -ex

pushd apps
riscv64-linux-gnu-gcc -g -Wall -static -o init init.c
riscv64-linux-gnu-gcc -g -Wall -static -o sh sh.c
riscv64-linux-gnu-gcc -g -Wall -static -o mount mount.c
riscv64-linux-gnu-gcc -g -Wall -static -o umount umount.c
riscv64-linux-gnu-gcc -g -Wall -static -o ls ls.c
riscv64-linux-gnu-gcc -g -Wall -static -o lazybox lazybox.c
popd

mkdir -p initramfs

pushd initramfs

mkdir -p bin
mkdir -p etc
mkdir -p root
mkdir -p proc

# optional
mkdir -p sys
mkdir -p run
mkdir -p dev

cat > root/hello.txt << "EOF"
Hello, My own Linux system!
EOF

cat <<EOF > etc/rc
#!/bin/sh
mount -t proc proc /proc

# optional
mount -t sysfs sysfs /sys
mount -t tmpfs tmpfs /run
mount -t devtmpfs devtmpfs /dev

# start an interactive shell
/bin/sh
EOF

pushd bin
cp ../../apps/init init
cp ../../apps/sh sh
cp ../../apps/mount mount
cp ../../apps/umount umount
cp ../../apps/ls ls
cp ../../apps/lazybox lazybox

test -L cat || ln -s lazybox cat
test -L tee || ln -s lazybox tee
test -L ps || ln -s lazybox ps
test -L poweroff || ln -s lazybox poweroff
popd

find . | \
     cpio -o -v --format=newc | \
     gzip > ../initramfs.cpio.gz
popd