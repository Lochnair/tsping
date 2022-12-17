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

## OpenWrt
Currently the Makefile for a package lives over here: https://github.com/Lochnair/openwrt-feeds/blob/main/tsping/Makefile
If you build your own OpenWrt image, add the repo as a feed and enable the package in the config.
Alternatively use the OpenWrt SDK to only compile this single package.
