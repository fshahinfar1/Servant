# About

These instructions are for preparing the cloudlab environment.

# Main Machine (Test machine)


1. Install Linux kernel 5.13 + rdtsc instructions

> This version of linux is not a requirement but I have done tests using this kernel

```bash
cd /proj/progstack-PG0/farbod/ubuntu
sudo dpkg -i *.deb
```

> The packages for installing the kernel is available at ebpf-bench repository.


2. Enable huge pages.

```bash
echo 'GRUB_CMDLINE_LINUX_DEFAULT="default_hugepagesz=1G hugepagesz=1G hugepages=16"' | sudo tee -a /etc/default/grub
sudo update-grub
sudo reboot
```

After rebooting the 1GB huge pages are available and the kerne should be 5.13.

```bash
uname -a
# Linux hostname 5.13.0-custom-ebpf-rdtsc #6 SMP Mon Sep 27 15:45:14 BST 2021 x86_64 x86_64 x86_64 GNU/Linux
```


3. Install `uBPF`

```bash
git clone https://fyro.ir/fshahinfar1/uBPF.git
cd uBPF/vm
make
sudo make install
```

> After these steps `libubpf` will be installed to the system.
Other projects can link to it using `-lbpf` flag.


4. Install `libbpf`

```bash
# Some dependencies you might need
sudo apt update
sudo apt install -y libz-dev libelf-dev pkg-config
# Getting and compiling the libbpf
git clone https://github.com/libbpf/libbpf
cd libbpf
git checkout f9f6e92458899fee5d3d6c62c645755c25dd502d
cd src
mkdir build
make all OBJDIR=.
# make install_headers DESTDIR=build OBJDIR=.
make install DESTDIR=build OBJDIR=.
sudo make install
# Update ldconfig files
echo `whereis libbpf` | awk '{print $2}' | xargs dirname | sudo tee /etc/ld.so.conf.d/libbpf.conf
sudo ldconfig
```

> For some reason building libbpf without pkg-config being installed cause some problems.


5. Get the servant

Install `llvm-13`. You need to get installation script from the website (also available in /docs directory).

```bash
git clone https://fyro.ir/fshahinfar1/Servant.git
cd Servant/src
git submodule update --init
make
```

6. Flow Steering

currently `Servant` only reads from one queue which is defined when running it.
Make sure your traffic is placed on that queue.

```
sudo ethtool  -U enp24s0f1 flow-type udp4 dst-port 8080 action 2
```

7. Busypolling

If you want to run `Servant` with `--busy-poll` on the same core that
is handling the interrupts, then you should defer the intrrupts.
Use following script to enable/disable busypolling configuration.

```bash
#! /bin/bash

dev=enp24s0f1

if [ "x$1" = "xoff" ]; then
	echo 0 | sudo tee /sys/class/net/$dev/napi_defer_hard_irqs
	echo 0 | sudo tee /sys/class/net/$dev/gro_flush_timeout
else
	echo 2 | sudo tee /sys/class/net/$dev/napi_defer_hard_irqs
	echo 200000 | sudo tee /sys/class/net/$dev/gro_flush_timeout
fi
```

8. Compiling lib-interpose

Use `make` command in the `src/interpose` directory. It will need
a version of `ubpf` that has been compiled with `-fPIC` flags.
a diff of changes for `ubpf` make file to compile for this purpose
is provided in this directory. apply the diff and make then install ubpf.

**Note:** ubpf with `-fPIC` flag is not used in the runtime. It is
only used in the lib-interpose.


9. Examples/Benchmarks

There are some examples in this repository (`/examples/` directory).
You can build them with `make`.


10. BMC

> Only If you want to experiment with BMC.

First install some dependenceis.

```bash
sudo apt update
sudo apt install -y clang-9 llvm-9
# Use llvm.sh script to install llvm-13
```
> Install `llvm-13`. You need to get installation script from the website (also available in /docs directory).

Then get the repository and build.

```bash
git clone https://fyro.ir/fshahinfar1/BMC
cd BMC
./make.sh
```

Note1:
You can run BMC with the `start.sh` script provided.

Note2:
This custom BMC repository has multiple branches. The main branch is a version
that runs in XDP. For running in XDP+uBPF you should checkout to other branches.
The `sevant-bmc-2` branch needs `servant_interpose.so` for injecting packets to
`Memcached`.


11. Katran

continue reading [here](./preparing_katran.md)


# Workload Generator machine


1. Setup Huge pages

Edit `/etc/default/grub` and update these variables

```bash
echo 'GRUB_CMDLINE_LINUX_DEFAULT="default_hugepagesz=1G hugepagesz=1G hugepages=16"' | sudo tee -a /etc/default/grub
sudo update-grub
sudo reboot
```


2. DPDK

```bash
sudo apt update
sudo apt install meson ninja-build python3-pyelftools libpcap-dev
git clone git://dpdk.org/dpdk
cd dpdk
git checkout v21.11
meson build
cd build
ninja
sudo ninja install
```


3. pktgen

```bash
sudo apt install libnuma-dev
git clone git://dpdk.org/apps/pktgen-dpdk
cd pktgen-dpdk
git checkout pktgen-21.11.0
meson build
cd build
ninja
sudo ninja install
```


4. Mutilate

If you need Muiltae get the install script and run it.
(Mainly for BMC experiments)


5. Get DPDK `igb_uio` Kernel Module

```bash
git clone git://dpdk.org/dpdk-kmods
cd dpdk-kmods/linux/igb_uio/
make
sudo modprobe uio
sudo insmod /users/farbod/dpdk-kmods/linux/igb_uio/igb_uio.ko
```

6. Bind Script for DPDK

> Take note of interface MAC address before binding!

```bash
#! /bin/bash

INTERFACE=enp24s0f1
PCI=18:00.1
DPDK_DRIVER=igb_uio
NORMAL_DRIVER=i40e
IP="192.168.1.2/24"

if [ "x$1" = "xunbind" ]; then
	sudo dpdk-devbind.py -u $PCI
	sudo dpdk-devbind.py -b $NORMAL_DRIVER $PCI
	sudo ip link set dev $INTERFACE up
	sudo ip addr add $IP dev $INTERFACE
else
	sudo ip link set dev $INTERFACE down
	sudo dpdk-devbind.py -u $PCI
	sudo dpdk-devbind.py -b $DPDK_DRIVER $PCI
fi
```


# Generating Load

## Memcached/BMC

Use stress2.sh script for loading and stressing Memcached.

## Katran

Use pktgen.
