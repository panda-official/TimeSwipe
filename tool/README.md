# Timeswipe Tools

## Timeswipe resampler (command line tool)

### Visualization with GNU Plot

#### Visualising with the `plot.sh` script

There is easy to use [plot.sh](plot.sh) Bash script to visualize the sensor data
with GNU Plot which can be used as follows:

```
plot.sh [-s sensor-number ...] [-S csv-separators] <input-file>
```

It's possible to specify `-s` option multiple times to plot the values from
multiple sensors. If omitted, the values from all 4 sensors will be visualized.

The option `-S` can be used to specify the CSV separators to be used. If omitted
the comma is used as the separator.

The `<input-file>` argument must be a valid path to binary or CSV file.

### Visualizing the CSV input interactively

Usually CSV files with the sensor data uses comma as the separator, so first set it:

```
set datafile separator comma
```

For better experience it's may helpful to the establish the total range by the contents
of the input file:

```
set autoscale x
```

To visualize the values of sensor 2 from a CSV file:

```
plot 'input.csv' using 2 with lines
```

To visualize the values of sensors 1 and 3 from a CSV file:

```
plot 'input.csv' using 1 with lines, '' using 3 with lines
```

#### Visualizing the binary input interactively

Usually binary files with the sensor data are just sequences of 32 bit floats:

```
float32float32...float32
```

Thus, the following declaration for `gnuplot` should be used: plot the data of
`Nth` column from binary file which contains 4 float columns but only the `Nth`
column is need to be read and 3 other columns must be discarded (described by
`format`); use the values of the desired column as the values of `Y` axis
(described by `using`).

To visualize the values of sensor 1 from a binary file with data of 4 sensors:

```
plot 'input.bin' binary array=(-1) format='%float%*3float' using 1 with lines
```

To visualize the values of sensor 2 from a binary file with data of 4 sensors:

```
plot 'input.bin' binary array=(-1) format='%*float%float%*2float' using 1 with lines
```

To visualize the values of sensor 4 from a binary file with data of 4 sensors:

```
plot 'input.bin' binary array=(-1) format='%*3float%float' using 1 with lines
```

To visualize the values of all sensors from a binary file with data of 4 sensors:

```
plot 'input.bin' binary array=(-1) format='%float%*3float' using 1 with lines,\
     '' binary array=(-1) format='%*float%float%*2float' using 1 with lines,\
     '' binary array=(-1) format='%*2float%float%*float' using 1 with lines,\
     '' binary array=(-1) format='%*3float%float' using 1 with lines
```
