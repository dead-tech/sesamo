![Build](https://github.com/dead-tech/sesamo/workflows/Build/badge.svg)

# Sesamo
Simple serial port monitor. This is supposed to be an alternative to cutecom.

![sesamo_overview](https://github.com/user-attachments/assets/9267dbaf-cd30-4e45-bff9-bf97211388d1)

__Disclaimer__: If builds are failing that is probably because the CI has not the correct version
of the standard library to compile the project. We are using some of the latest features therefore
you might not be able to compile the project yourself.

## Download
Check out the releases page.


## Build
Currently the only way to use this tool is to build it yourself.
```
$ git clone https://github.com/dead-tech/sesamo.git
$ cd sesamo
$ mkdir build
$ cmake -S . -B build
$ cmake --build build
```

## Usage
The UI is pretty self-explanatory. What you may be interested in is keyboard shortcuts:
- Enter  -> Connect
- Q      -> Disconnect
- Ctrl+L -> Clear read buffer
