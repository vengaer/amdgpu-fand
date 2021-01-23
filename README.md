# amdgpu-fand

A daemon managing the fan speed of AMD Radeon graphics cards. Supports both glibc- and musl based systems.  

## Dependencies

- mesa
- clang (optional, for testing)
- llvm toolchain (optional, for testing)
- graphviz (optional, for documentation)

## Installation

The daemon may be installed either by building it manually or, for Arch-based distros, by using the supplied PKGBUILD. No matter which approach
is chosen, the daemon itself, `amdgpu-fand`, its control interface `amdgpu-fanctl`, a default config file `/etc/amdgpu-fand.conf` and a systemd
service `/etc/systemd/system/amdgpu-fand.service` are installed.    

Once installed, the systemd service may be started and/or enabled using `systemctl start/enable amdgpu-fand`.  

#### Building Manually

```sh
git clone https://gitlab.com/vilhelmengstrom/amdgpu-fand
cd amdgpu-fand
make release
sudo make install
```

#### PKGBUILD

```sh
git clone https://gitlab.com/vilhelmengstrom/amdgpu-fand
cd amdgpu-fand/config/pkg
makepkg -si
```

Compared to building manually, this has the advantage of making the binaries managed by pacman. It will also avoid overwriting modified
configuration files.  


## Configuration

The daemon is configured via `/etc/amdgpu-fand.conf`. The options themselves are fairly simple and describe only how the daemon should manage
the fans, no info is required to detect the graphics card itself.  

#### Interval

The interval in seconds with which the fan speed should be updated based on the card's temperature.   

Valid settings: 0-65535  

#### Hysteresis

The hysteresis setting provides a means of delaying the reduction of fan speed until the temperature has fallen far enough. This allows for avoiding the
fan speed rapidly fluctuating around a threshold in the speed matrix.    

As an example, assume there is a threshold at 60 degrees Celsius specified in the speed matrix and that a hysteresis of 3 is used. Once the temperature
is increasing and reaches 60 degrees, the daemon will immediately (subject only to the interval) set the fan speed according to what is specified in the
speed matrix. If the temperature then falls below 60 degrees, the daemon will delay slowing the fan(s) down until the temperature has hit 57 (60 - 3) 
degrees. Instead, it will keep the fan(s) spinning with the speed specified for the 60-degree threshold until the temperature has fallen far enough.  

Valid settings: 0-255  

#### Aggressive Throttle

This allows for minimizing the fan speed as soon as the temperature falls below the lowest threshold specified in the speed matrix. If the card supports
turning the fan(s) off completely, the daemon will do so. When set to false, the fan(s) will instead spin with the speed specified for the lowest threshold 
of the speed matrix.  

Valid settings: true, false  

#### Matrix

The matrix defines the temperature-speed relation for the fan curve. The first column contains the temperature and the second the speed. At most 16 rows
may be supplied. The start of the matrix is denoted using an opening parenthesis [(] and the end by a closing one [)]. Temperature-speed pairs are
given as single-quoted strings, the values separated by two colons [::].

<pre>
Valid setting (example): ('50::0'  
                          '60::30'  
                          '70::70')  
</pre>

The numeric limits supported by the daemon for temperatures are 0-255 degrees Celsius (although the card would obviously melt far below the upper limit). The speeds
are given as percentages (0-100).  

## Control Interface

The daemon comes with a separate control interface, `amdgpu-fanctl`. This may be used to query the daemon for the current speed, temperature and matrix using the
`-g speed`, `-g temp` and `-g matrix` options, respectively. It may also be used to terminate the daemon using the `-e` switch. For security reasons, the latter
requires root access.  

If the daemon is terminated, it will first relinquish control of the fans to the kernel.  

## Build Options

There are a number of slightly more obscure options that can be specified, both for building the binaries and for testing them.  

*NOTE*: The unit test and fuzzing code relies on modifying symbols in relocatable ELF objects in order to mock out certain functions. By modifying the
compiler and/or linker flags, the original symbols may unintentionally end up being linked into the binary instead, causing some of the tests to fail.

#### DRM 

By default, the daemon uses the DRM subsystem of the kernel for querying the card for its temperature. This is done assuming that the `libdrm/amdgpu_drm.h` header can be
found on the system when building. If this is not the case, the daemon will instead use files under sysfs for reading the temperature. If wanting to use sysfs even
if DRM is available, the behavior may be overwritten by passing `drm_support=n` to `make` when building.  

#### Unit Tests

The unit tests can be built and run using  

```sh
make testrun -B
```

#### Fuzzing

There are currently four different interfaces that are fuzzed, three of which are exposed by `amdgpu-fand` and one by `amdgpu-fanctl`. If wanting to fuzz an interface,
firstly ensure that clang and the llvm toolchain are installed. Then run    

```sh
make fuzzrun FUZZIFACE=INTERFACE FUZZTIME=TIME -B
```

INTERFACE may be one of `client`, `server`, `cache` and `config` which correspond to fuzzing of fanctl's IPC client code and the daemon's IPC server, caching and config parsing code,
respectively. TIME is the number of seconds the fuzzer is to be run.  

## Disclaimer

This project is in no way associated with AMD.

