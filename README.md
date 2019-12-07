# amdgpu-fand

A Systemd daemon that automatically manages the fan speed of Radeon graphics cards. 

Provides the daemon itself, `amdgpu-fand`, and its control interface `amdgpu-fanctl`.

[![Build Status](https://gitlab.com/vilhelmengstrom/amdgpu-fand/badges/master/build.svg)](https://gitlab.com/vilhelmengstrom/amdgpu-fand/commits/master)

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

The daemon is copied to `/usr/local/bin/amdgpu-fand`, the control interface to `/usr/local/bin/amdgpu-fanctl`, 
the default config to `/etc/amdgpu-fand.conf` and the systemd service to `/etc/systemd/system/amdgpu-fand.service`  

## Daemon

`amdgpu-fand` will continuously monitor the temperature of the chip and adjust the fan speed accordingly. Settings 
are read primarily from the config but some may be overridden using the control interface.

### Configuration

Configurations are read from `/etc/amdgpu-fand.conf` unless overridden with the `-c` switch (see amdgpu-fand --help). 
Most options should hopefully be self-explanatory but some may require a bit of explanation.

##### Persistent Path

The persistent path is the location of the graphics card's hardware monitors in sysfs. The configure script sets this by following the
symlink `/sys/class/drm/card0`. This, and other symlinks in sysfs, may change between boots, hence why the persistent path is used. The 
option can be left empty, in which case the program will itself follow the `card0` symlink.   

To get the persistent path for card0, run `readlink -f /sys/class/drm/card0`.

##### Hwmon

Here the desired hardware monitor of the card can be specified. If left blank, the program will detect it by itself. As, in my somewhat limited experience,
most cards seem to have only one, this is likely not something that needs to be tweaked by most people.  

To list available hardware monitors for card0, run `ls /sys/class/drm/card0/device/hwmon | grep hwmon[0-9]`.

##### Fan Curves

The fan curves can be customized via discrete points set in `/etc/amdgpu-fand.conf`. The values are interpolated with respect tot he temperature of the card. Temperatures and speeds are given in a matrix whose first column contains the temperature and the second the desired speed percentage. The columns are separated by 2 colons (::). The max number of rows is 16.

In addition to specifying sample pointer, the interpolation used can also be chosen. The options are linear and cosine. Additionally, aggressive throttling may be set. If the latter it enabled, the fan speed will be set to the lowest speed in the matrix as soon as the temperature falls below the second lowest.

## Control Interface

The daemon may be interacted with using its control interface `amdgpu-fanctl`. This allows for fetching values such as current temperature and fan speed without
having to find them in sysfs. Additionally, the fan speed may be overridden using the set command. See amdgpu-fanctl --help for a full list of available options.

#### The Get Command

Using the control interface's get command to get e.g. temperature and fan speed has two significant advantages as opposed to finding them in sysfs manually. Firstly,
the values gotten from the sysfs tachometer interface are not guaranteed to be correct (as observed by both myself and [others](https://www.reddit.com/r/Amd/comments/9b0nmy/linuxamdgpu_rx_580_fan_always_on_windows_usually/e4zqah0/?utm_source=share&utm_medium=web2x)). This is handled internally by `amdgpu-fand`, meaning that `amdgpu-fanctl get speed` will always return the correct value.

Secondly, and perhaps more important, the sysfs interface seems to handle multiple processes interfacing with it poorly (I have yet to find a conclusive reason as to why but it is seemingly tied to concurrent access). This may manifest itself as the processes being unable to open the files which will essentially lock the threads running them. Using `amdgpu-fanctl`, all accesses to the relevant files is sequentialized, avoiding the issues. Alternatively, `amdgpu-fand` places exclusive advisory locks on the sysfs files it opens, meaning that `flock` should be another option if wanting to interface with sysfs directly.

#### The Set Command

The control interface allows you to override the fan speed using `amdgpu-fanctl set speed PERCENTAGE`. For safety reasons, such an override is, by default, tied to the process (i.e. the shell instance) that invoked the `amdgpu-fanctl` command. Once that process is killed, the speed override will be reset. This is to avoid potential issues such as forcing the speed to 0%, forgetting that this has been done and starting up something GPU-intensive only to have the card melt. This behaviour can be overridden using the `--detach` switch to `amdgpu-fanctl`, in which case the fan speed will remain what was set via the control interface until either the daemon is restarted or `amdgpu-fanctl reset speed` is run.

## Disclaimer  
This piece of software is not affiliated with AMD in any way. Radeon is a trademark of AMD.

## Other Options  
These repos contain projects achieving the same goal:

- [amdgpu-fan](https://github.com/chestm007/amdgpu-fan)
- [amdgpu-fancontrol](https://github.com/grmat/amdgpu-fancontrol)
