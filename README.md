### Richard Elling's ZFSonLinux tools

This is a collection of miscellaneous tools useful for 
[ZFSonLinux](http://zfsonlinux.org/) environments.

#### zfetchstat
Monitors the efficiency of the ZFS intelligent prefetcher.

#### kstat-analyzer
This is a port of a Solaris kstat analysis tool. This tool currently
reports on ZFSonLinux kstats available in `/proc/spl/kstat.` If you've
ever contemplated running `arc_summary,` I think you'll find the
`kstat-analyzer` to be more comprehensive and easier to digest.

#### fio2influx
Parses fio JSON logs and prepare as influxdb line protocol. In the 
UNIX tradition, the output is expected to be piped somewhere.
