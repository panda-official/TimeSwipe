# rPIspi Example Application

This section describes how to compile the `rPIspi` (SPI terminal) example application using the TimeSwipe driver.
Supported operating systems are Raspbian Buster and Arch Linux ARMv8 AArch64.


## Raspbian Buster

Navigate to the `rPIspi` directory:

```
cd timeswipe/driver/examples/rPIspi
```

You can then build the application with `cmake`:

```
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

To run the SPI terminal example from the `build` directory, execute the command:

```
sudo ./rPIspi 0
```

This will open a `SPI` terminal to the TimeSwipe board on `SPI` bus 0.
Attach commands to run in a non-interactive mode, answers will be printed on `stdout`.
`Ctrl + c` exits the application.


## Arch Linux ARMv8 AArch64

Navigate to the `rPIspi` directory:

```
cd timeswipe/driver/examples/rPIspi
```

You can then build the application with `cmake`:

```
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

To run the SPI terminal example from the `build` directory, as `root` execute the command:

```
./rPIspi
```

This will open a `SPI` terminal to the TimeSwipe board on `SPI` bus 0.
Attach commands to run in a non-interactive mode, answers will be printed on `stdout`.
`Ctrl + c` exits the application.
