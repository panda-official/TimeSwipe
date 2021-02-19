#!/bin/bash
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin

err()
{
  echo $* >&2
  exit 1
}

usage()
{
  err 'Usage: plot.sh [-L] [-s sensor-number ...] [-S csv-separators] <input-file> ...'
}

while getopts Ls:S: opt
do
  case $opt in
    L) with_lines=no ;;
    s)
      num_re='^[0-9]+$'
      if [[ ! $OPTARG =~ $num_re || $OPTARG -lt 1 || $OPTARG -gt 4 ]]; then
        err 'plot.sh: invalid sensor number'
      fi
      sensor_numbers="$sensor_numbers $OPTARG" ;;
    S) csv_separators=$OPTARG ;;
    ?) usage ;;
  esac
done
shift `expr $OPTIND - 1`

if [ $# -lt 1 ]; then
  usage
fi

if [ X"$with_lines" != X'no' ]; then
  with_lines='with lines'
else
  with_lines=
fi

if [ X"$sensor_numbers" = X'' ]; then
  sensor_numbers='1 2 3 4'
fi

if [ X"$csv_separators" = X'' ]; then
  csv_separators=' \t,'
fi

ascii_re='^ASCII'
plot=
for input_file in "$@"; do
  if [[ ! -f "$input_file" ]]; then
    err "File \"$input_file\" not found"
  fi
  input_format=$(file -b "$input_file")
  fmt=
  if [ X"$input_format" = X'data' ]; then
    fmt='bin'
    s1bin="'$input_file' binary array=(-1) format='%float%*3float' using 1 $with_lines"
    s2bin="'$input_file' binary array=(-1) format='%*float%float%*2float' using 1 $with_lines"
    s3bin="'$input_file' binary array=(-1) format='%*2float%float%*float' using 1 $with_lines"
    s4bin="'$input_file' binary array=(-1) format='%*3float%float' using 1 $with_lines"
  elif [[ $input_format =~ $ascii_re ]]; then
    fmt='csv'
    s1csv="'$input_file' using 1 $with_lines"
    s2csv="'$input_file' using 2 $with_lines"
    s3csv="'$input_file' using 3 $with_lines"
    s4csv="'$input_file' using 4 $with_lines"
    if [[ -z "$before_plot" ]]; then
      before_plot="set term qt size 1366, 768; set datafile separator \"${csv_separators}\";set autoscale x"
    fi
  else
    err 'Unsupported input format'
  fi

  for sn in $sensor_numbers; do
    var=s${sn}${fmt}
    plot="$plot ${!var},"
  done
done

cmd="${before_plot}; plot ${plot:0:-1}"
#echo $cmd
gnuplot -p -e "$cmd"
