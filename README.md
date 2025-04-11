# Sesamo
Simple serial port monitor. This is supposed to be an alternative to cutecom.

![sesamo_overview](https://github.com/user-attachments/assets/9267dbaf-cd30-4e45-bff9-bf97211388d1)


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
