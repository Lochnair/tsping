# tsping

## Compilation
Prerequisites: You'll need a C compiler, Meson and Ninja installed

```bash 
git clone https://github.com/Lochnair/tsping.git
cd tsping
meson build builddir
cd builddir
meson compile
```

You should now have a 'tsping' binary in builddir.

## Usage
```
# tsping --help
Usage: tsping [OPTION...] IP1 IP2 IP3 ...
tsping -- a simple application to send ICMP echo/timestamp requests

  -e, --icmp-echo            Use ICMP echo requests
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
Alternatively use the OpenWrt SDK to only compile this single package.
