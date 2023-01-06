# tsping

## Compilation
Prerequisites: You'll need a C compiler, Meson and Ninja installed

```bash 
git clone https://github.com/Lochnair/tsping.git
cd tsping
meson setup builddir
cd builddir
meson compile
```

You should now have a 'tsping' binary in builddir.

## Usage
```
# tsping --help
Usage: tsping [OPTION...] IP1 IP2 IP3 ...
tsping -- a simple application to send ICMP echo/timestamp requests

  -D, --print-timestamps     Print UNIX timestamps for responses
  -e, --icmp-echo            Use ICMP echo requests
  -f, --fw-mark=MARK         Firewall mark to set on outgoing packets
  -i, --interface=INTERFACE  Interface to bind to
  -m, --machine-readable     Output results in a machine readable format
  -r, --target-spacing=TIME  Time to wait between pinging each target in ms
                             (default 0)
  -s, --sleep-time=TIME      Time to wait between each round of pinging in ms
                             (default 100)
  -t, --icmp-ts              Use ICMP timestamp requests (default)
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```

## OpenWrt
Currently the Makefile for a package lives over here: https://github.com/Lochnair/openwrt-feeds/blob/main/tsping/Makefile
If you build your own OpenWrt image, add the repo as a feed and enable the package in the config.

Alternatively use the OpenWrt SDK to only compile this single package, example below:

```bash
wget https://downloads.openwrt.org/releases/22.03.2/targets/octeon/generic/openwrt-sdk-22.03.2-octeon_gcc-11.2.0_musl.Linux-x86_64.tar.xz
tar xf openwrt-sdk-22.03.2-octeon_gcc-11.2.0_musl.Linux-x86_64.tar.xz
cd openwrt-sdk-22.03.2-octeon_gcc-11.2.0_musl.Linux-x86_64
cp -v feeds.conf.default feeds.conf
echo 'src-git-full lochnair https://github.com/Lochnair/openwrt-feeds.git' >> feeds.conf
./scripts/feeds update -a
./scripts/feeds install -a
make package/tsping/compile
file bin/packages/mips64_octeonplus/lochnair/tsping_0.1-1_mips64_octeonplus.ipk
```
