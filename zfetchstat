#!/usr/bin/env python
"""
zfetchstat - observes the DMU intelligent prefetcher statistics for
ZFS on Linux systems http://www.ZFSonLinux.org

The statistics are available from the /proc/spl/kstat/zfs/zfetchstats file.

Note: Solaris fans might recognize this as a python port to the perl version
of zfetchstat, but there are fewer statistics available to the ZFSonLinux port.

Copyright 2015-2018 Richard Elling <Richard.Elling@RichardElling.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""
from __future__ import print_function

import sys
from datetime import datetime
from time import sleep
try:
    # noinspection PyUnresolvedReferences
    from argparse import ArgumentParser, ArgumentTypeError
except ImportError as exc:
    print('error: {}'.format(exc))
    exit(1)

ROW_FORMAT = '{:>12} {:>12} {:>8} {:>16}'
TIME_FORMAT = '{:>26}'
VALUES = ['hits', 'misses', 'max_streams']


def parse_options(args_list):
    """
    parse command line options

    :param args_list: arguments passed as list (eg sys.argv[1:])
    :type args_list: list
    :return: options
    :rtype: optargs
    """
    parser = ArgumentParser(
        description='Display ZFS DMU prefetcher statistics'
    )
    parser.add_argument('-E', '--no_epoch', action='store_true',
                        help='do not print rates from epoch as first line')
    parser.add_argument('-H', '--no_header', action='store_true',
                        help='do not print column headers')
    parser.add_argument('-T', '--no_timestamp', action='store_true',
                        help='do not time column')
    parser.add_argument('-f', '--filename', type=str,
                        default='/proc/spl/kstat/zfs/zfetchstats',
                        help='file containing zfetchstats statistics')
    parser.add_argument('interval', type=check_positive_float, default=1.0,
                        nargs='?',
                        help='sample interval in seconds (default=1.0)')
    parser.add_argument('count', type=check_positive_int, default=sys.maxsize,
                        nargs='?',
                        help='number of intervals (default=infinite)')
    return parser.parse_args(args_list)


def check_positive_int(value):
    """
    ensure argparse int value is a positive value > 0

    :param value: proposed value
    :return: approved value
    :rtype: int
    """
    try:
        val = int(value)
    except ValueError:
        val = 0
    if val < 1:
        raise ArgumentTypeError(
            '{} must be a positive integer > 0'.format(value))
    return val


def check_positive_float(value):
    """
    ensure argparse float value is a positive value > 0

    :param value:
    :return:
    """
    try:
        val = float(value)
    except ValueError:
        val = 0.0
    if val <= 0.0:
        raise ArgumentTypeError(
            '{} must be a positive value > 0.0'.format(value))
    return val


def print_header(no_timestamp=False):
    """
    print header

    :param no_timestamp: if True, do not print timestamp
    :type no_timestamp: bool
    """
    if not no_timestamp:
        print(TIME_FORMAT.format('time'), end='')
    print(
        ROW_FORMAT.format('hits/sec', 'misses/sec', 'hit%', 'max_streams/sec'))


def print_values(previous, current, no_timestamp=False):
    """
    print values

    :param previous: last sample
    :type previous: dict
    :param current: current sample
    :type current: dict
    :param no_timestamp: if True, do not print timestamp
    :type no_timestamp: bool
    """
    fmt = '{:.2f}'
    delta_v = {'timestamp': current['timestamp']}
    delta_t = float(current['timestamp'] - previous['timestamp']) / 1e9
    if delta_t < 1e-9:
        # no forward progress in time, this shouldn't happen except in test
        delta_t = 1.0
    for s in VALUES:
        delta_v[s] = current[s] - previous[s]
        delta_v[s + '_rate'] = float(delta_v[s]) / delta_t

    accesses = delta_v['hits'] + delta_v['misses']
    if accesses == 0:
        hit_percent = 0.0
    else:
        hit_percent = (delta_v['hits'] * 100.0) / accesses

    # the timestamp here is walltime at the time of printing, it is not
    # the time when the stats were collected, though it should be close
    if not no_timestamp:
        print(TIME_FORMAT.format(datetime.now().isoformat()), end='')
    print(
        ROW_FORMAT.format(
            fmt.format(delta_v['hits_rate']),
            fmt.format(delta_v['misses_rate']),
            fmt.format(hit_percent),
            fmt.format(delta_v['max_streams_rate'])
        )
    )


def init_sample():
    """
    initialize sample dict

    :return: initialized sample
    :rtype: dict
    """
    d = {'timestamp': 0}
    for s in VALUES:
        d[s] = 0
    return d


def main():
    """
    collect and print stats until finished

    :return: exit code
    :rtype: int
    """
    options = parse_options(sys.argv[1:])
    count = options.count
    first_pass = True
    previous = init_sample()
    while count >= 0:
        if not first_pass:
            sleep(options.interval)
        current = init_sample()
        try:
            with open(options.filename, 'r') as f:
                current['timestamp'] = int(f.readline().split()[-1])
                for line in f.readlines():
                    s = line.split()
                    if s[0] in VALUES:
                        current[s[0]] = int(s[-1])
        except ValueError:
            print('error: cannot parse {}'.format(options.filename))
            return 1
        except IOError as io_exc:
            print('error: {}'.format(io_exc))
            return 1

        if first_pass:
            if not options.no_header:
                print_header(no_timestamp=options.no_timestamp)
            if not options.no_epoch:
                print_values(previous, current,
                             no_timestamp=options.no_timestamp)
            first_pass = False
        else:
            print_values(previous, current, no_timestamp=options.no_timestamp)
        previous = current
        count -= 1
    return 0


if __name__ == '__main__':
    try:
        res = main()
    except KeyboardInterrupt:
        sys.exit(0)
    sys.exit(res)
