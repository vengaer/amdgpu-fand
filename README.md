# amdgpu-fanctl  

A Systemd daemon that automatically manages the fan speed of Radeon graphics cards.  

[![Build Status](https://gitlab.com/vilhelmengstrom/amdgpu-fanctl/badges/master/build.svg)](https://gitlab.com/vilhelmengstrom/amdgpu-fanctl/commits/master)

### Dependencies  

- AMDGPU -- The open source AMD Radeon graphics driver. On Arch-based distros this is part of the mesa package  

### Installation  

```
make   
sudo make install   
```

This will copy the executable to /usr/local/bin/amdgpu-fanctl, the default config to /etc/amdgpu-fanctl.conf
and the systemd service to /etc/systemd/system/amdgpu-fanctl.service  

### Disclaimer  
This piece of software is not affiliated with AMD in any way. Radeon is a trademark of AMD.

### Other Options  
These repos contain projects achieving the same goal:

- [amdgpu-fan](https://github.com/chestm007/amdgpu-fan)
- [amdgpu-fancontrol](https://github.com/grmat/amdgpu-fancontrol)
