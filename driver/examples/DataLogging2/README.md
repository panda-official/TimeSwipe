# DataLogging Example Application

This section describes how to compile the `DataLogging` example application using the TimeSwipe driver.
Supported operating systems are Raspbian Buster and Arch Linux ARMv8 AArch64.


## Raspbian Buster

Navigate to the `DataLogging` directory:

```
cd timeswipe/driver/examples/DataLogging
```

You can then build the application with `cmake`:

```
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

To run the data logging example from the `build` directory, execute the command:

```
sudo ./main --config ../config.json --input IEPE --output temp.txt
```

This will gather data for 10 seconds according to the configuration file specified, from the `IEPE` inputs and will save the data in CSV format to the file `temp.txt`.


## Arch Linux ARMv8 AArch64

Navigate to the `DataLogging` directory:

```
cd timeswipe/driver/examples/DataLogging
```

You can then build the application with `cmake`:

```
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

To run the data logging example from the `build` directory, as `root` execute the command:

```
./main --config ../config.json --input IEPE --output temp.txt
```

This will gather data for 10 seconds according to the configuration file specified, from the `IEPE` inputs and will save the data in CSV format to the file `temp.txt`.

