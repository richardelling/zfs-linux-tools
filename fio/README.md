# Tools helpful with using fio for performance characterization

#### fio2influx.py
Takes fio JSON output and parses out the interesting throughput measurements for
feeding into influxdb or telegraf. The timestamp reported is parsed from the file
itself, so you can collect a batch of results and send them to influxdb
at a later time, but still get correlation to other data in influxdb.

The intent is to take fio json output and publish thusly:
```
   fio --output=fio-results.json --output-format=json ...
   fio2influx fio-results.json | nc influx-telegraf-host port
```
An example of multiclient random read test delivering >1M IOPS:
```
fio,bs=4k,filename=../test/fio-6hosts-1511771394.json,iodepth=32,ioengine=libaio,job_hostname=host152,jobname=host152,name=random,numjobs=4,rw=randread,size=100g read.bw=936086,read.io_bytes=112331352,read.iops=234021.699819,read.runtime=120001,sys_cpu=22.639375,usr_cpu=5.972083,write.bw=0,write.io_bytes=0,write.iops=0.0,write.runtime=0 1511771394000000000
fio,bs=4k,filename=../test/fio-6hosts-1511771394.json,iodepth=32,ioengine=libaio,job_hostname=host154,jobname=host154,name=random,numjobs=4,rw=randread,size=100g read.bw=735600,read.io_bytes=88273520,read.iops=183900.101665,read.runtime=120002,sys_cpu=17.127357,usr_cpu=6.987442,write.bw=0,write.io_bytes=0,write.iops=0.0,write.runtime=0 1511771394000000000
fio,bs=4k,filename=../test/fio-6hosts-1511771394.json,iodepth=32,ioengine=libaio,job_hostname=host132,jobname=host132,name=random,numjobs=4,rw=randread,size=100g read.bw=824884,read.io_bytes=98986976,read.iops=206221.148157,read.runtime=120001,sys_cpu=38.630417,usr_cpu=5.902292,write.bw=0,write.io_bytes=0,write.iops=0.0,write.runtime=0 1511771394000000000
fio,bs=4k,filename=../test/fio-6hosts-1511771394.json,iodepth=32,ioengine=libaio,job_hostname=host130,jobname=host130,name=random,numjobs=4,rw=randread,size=100g read.bw=538243,read.io_bytes=64589804,read.iops=134560.970325,read.runtime=120001,sys_cpu=26.437083,usr_cpu=5.026875,write.bw=0,write.io_bytes=0,write.iops=0.0,write.runtime=0 1511771394000000000
fio,bs=4k,filename=../test/fio-6hosts-1511771394.json,iodepth=32,ioengine=libaio,job_hostname=host133,jobname=host133,name=random,numjobs=4,rw=randread,size=100g read.bw=826108,read.io_bytes=99133844,read.iops=206527.120607,read.runtime=120001,sys_cpu=37.808958,usr_cpu=6.944792,write.bw=0,write.io_bytes=0,write.iops=0.0,write.runtime=0 1511771394000000000
fio,bs=4k,filename=../test/fio-6hosts-1511771394.json,iodepth=32,ioengine=libaio,job_hostname=host131,jobname=host131,name=random,numjobs=4,rw=randread,size=100g read.bw=533554,read.io_bytes=64027100,read.iops=133388.680094,read.runtime=120001,sys_cpu=41.084792,usr_cpu=5.230625,write.bw=0,write.io_bytes=0,write.iops=0.0,write.runtime=0 1511771394000000000
fio,bs=4k,filename=../test/fio-6hosts-1511771394.json,iodepth=32,ioengine=libaio,job_hostname=host131,jobname=All\ clients,name=random,numjobs=4,rw=randread,size=100g read.bw=4394448,read.io_bytes=527342596,read.iops=1098612.098132,read.runtime=120002,sys_cpu=30.621312,usr_cpu=6.010686,write.bw=0,write.io_bytes=0,write.iops=0.0,write.runtime=0 1511771394000000000
```