# amdgpu-fanctl  

A Systemd daemon that automatically manages the fan speed of Radeon graphics cards.  

[![Build Status](https://gitlab.com/vilhelmengstrom/amdgpu-fanctl/badges/master/build.svg)](https://gitlab.com/vilhelmengstrom/amdgpu-fanctl/commits/master)

## Dependencies  

- AMDGPU -- The open source AMD Radeon graphics driver. On Arch-based distros this is part of the mesa package  

## Installation  

```
./configure
make
sudo make install   
```

The configure script attempts to find the persistent location of the card's hardware monitor in sysfs. If 
multiple cards are present, the user is prompted to choose one.

The executable is copied to `/usr/local/bin/amdgpu-fanctl`, the default config to `/etc/amdgpu-fanctl.conf`
and the systemd service to `/etc/systemd/system/amdgpu-fanctl.service`  

## Configuration

Configurations are read from `/etc/amdgpu-fanctl.conf` unless overridden with the `-c` switch (see amdgpu-fanctl --help). 
Most options should hopefully be self-explanatory but some may require a bit of explanation.

#### Persistent Path

The persistent path is the location of the graphics card's hardware monitors in sysfs. The configure script sets this by following the
symlink `/sys/class/drm/card0`. This, and other symlinks in sysfs, may change between boots, hence why the persistent path is used. The 
option can be left empty, in which case the program will itself follow the `card0` symlink.   

To get the persistent path for card0, run `readlink -f /sys/class/drm/card0`.

#### Hwmon

Here the desired hardware monitor of the card can be specified. If left blank, the program will detect it by itself. As, in my somewhat limited experience,
most cards seem to have only one, this is likely not something that needs to be tweaked by most people.  

To list available hardware monitors for card0, run `ls /sys/class/drm/card0/device/hwmon | grep hwmon[0-9]`.

#### Fan Curves

the fan curves can be customized via discrete points set in `/etc/amdgpu-fanctl.conf`. The values are interpolated with respect tot he temperature of the card. Temperatures and speeds are given in a matrix whose first column contains the temperature and the second the desired speed percentage. The columns are separated by 2 colons (::). The max number of rows is 16.

In addition to specifying sample pointer, the interpolation used can also be chosen. The options are linear and cosine. Additionally, aggressive throttling may be set. If the latter it enabled, the fan speed will be set to the lowest speed in the matrix as soon as the temperature falls below the second lowest.

## Disclaimer  
This piece of software is not affiliated with AMD in any way. Radeon is a trademark of AMD.

## Other Options  
These repos contain projects achieving the same goal:

- [amdgpu-fan](https://github.com/chestm007/amdgpu-fan)
- [amdgpu-fancontrol](https://github.com/grmat/amdgpu-fancontrol)
