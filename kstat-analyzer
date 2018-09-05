#!/usr/bin/env python
#
# ZFS on Linux kstat analyzer
#
# Copyright 2015-2018 Richard.Elling
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
from __future__ import print_function
import sys
import os
import locale
from datetime import datetime
from operator import itemgetter
from argparse import ArgumentParser

VERSION = '0.7.9'  # version is convenient to test against ZoL version

# ratios (%/100) for thresholds of cache analysis
CACHE_RATIO_OK = 0.25  # below OK: more analysis needed
CACHE_RATIO_GOOD = 0.9  # goodness threshold
CACHE_RATIO_EXCELLENT = 0.98  # excellence threshold
GHOST_RATIO_OK = 0.1  # above ghost hit ratio: more analysis needed
EVICTED_RATIO_OK = 0.5  # above evictions ratio: more analysis needed
PREFETCH_RATIO_OK = 0.5  # below prefetch hit ratio: more analysis needed

# handy constants
BYTES_PER_KIB = pow(2, 10)
BYTES_PER_MIB = pow(2, 20)
BYTES_PER_GIB = pow(2, 30)


def parse_args():
    """
    parse arguments

    :return: dict
    """
    parser = ArgumentParser()
    parser.add_argument('-a', '--analysis',
                        help='comma-separated list of analyses to perform, '
                             '--list for list')
    parser.add_argument('-l', '--list', action='store_true',
                        help='list available analyzers')
    parser.add_argument('-t', '--top',
                        help='number of entries in top/bottom lists (default=10)',
                        default=10)
    parser.add_argument('directory', nargs='*',
                        default=['/proc/spl'],
                        help='directory containing kstats (default=/proc/spl)')
    parser.add_argument('-d', '--debug', action='store_true',
                        help='enable debugging')
    args = parser.parse_args()
    return args


def preamble():
    """
    print preamble
    """
    name = 'ZFS on Linux Statistics Analyzer'
    print('#' * len(name))
    print(name)
    print('#' * len(name))
    print('Analysis date = {}Z'.format(datetime.utcnow().isoformat()))
    print('Analyzer version = {}'.format(VERSION))
    for i in options.directory:
        print('Directories analyzed: {}'.format(','.join(options.directory)))


def section_preamble(kstats, desc):
    """
    print preamble for a section

    :param kstats: kstats collected
    :type kstats: dict
    :param desc: section description
    :type desc: str
    """
    print('\n#### {}'.format(desc))
    if 'report_source' in kstats:
        print('Source directory = {}'.format(kstats['report_source']))


# analyzers
def arc_summary(kstats):
    """
    ZFS ARC usage summary

    :param kstats: parsed kstats
    :type kstats: dict
    """
    global options
    section_preamble(kstats, 'ZFS ARC analysis')
    if not kstats:
        pr_error('no data available')
        return

    # variables we refer to often and must exist
    for i in ['size', 'c', 'c_max', 'c_min', 'p', 'hits', 'misses']:
        if i not in kstats:
            pr_error('arcstats value \"{}\" not found'.format(i))
            return

    pr_desc(0, 'ARC Sizes')
    current_arc_size = float(kstats.get('size', 0))
    pr_desc(1, 'Current size (GiB) =', s_float(current_arc_size, scale=BYTES_PER_GIB))
    pr_desc(1, 'Max target size (GiB) = ', s_float(kstats['c_max'], scale=BYTES_PER_GIB))
    if 'overhead_size' in kstats:
        pr_desc(1, 'Overhead size (GiB) =',
                s_float(kstats['overhead_size'], scale=BYTES_PER_GIB))

    if 'anon_size' in kstats:
        pr_desc(1, 'Anonymous buffer size (GiB) =',
                s_float(kstats['anon_size'], scale=BYTES_PER_GIB))

    # print ARC size breakout as table
    check_size = 0
    table = [{'name': 'total', 'size': kstats.get('size', 0)}]
    for i in ['hdr_size', 'data_size', 'metadata_size', 'l2_hdr_size',
              'dbuf_size', 'dnode_size', 'bonus_size']:
        if i in kstats:
            table.append({'name': i.replace('_size', ''), 'size': kstats[i]})
            check_size += kstats[i]

    if options.debug and kstats['size'] != check_size:
        pr_error('ARC size != check_size: {} != {}'.format(kstats['size'], check_size))

    pr_desc(1, 'ARC size breakout')
    fmt = '{:10s} {:>10s} {:>11s}'
    pr_desc(2, fmt.format('use', 'size (GiB)', '% of total'))
    pr_desc(2, fmt.format(10 * '-', 10 * '-', 11 * '-'))
    for i in sorted(table, key=itemgetter('size'), reverse=True):
        pr_desc(2, fmt.format(
            i['name'],
            s_float(i['size'], scale=BYTES_PER_GIB),
            s_pct(i['size'], scale=current_arc_size, parens=False)))
    print()

    if 'arc_meta_used' in kstats:
        ds = kstats.get('data_size', 1)
        pr_desc(1, 'Metadata current size (GiB) =',
                s_float(kstats['arc_meta_used'], scale=BYTES_PER_GIB),
                s_pct(kstats['arc_meta_used'], scale=ds, of='data size'))
        if 'arc_meta_max' in kstats:
            pr_desc(2, 'Metadata max size observed (GiB) =',
                    s_float(kstats['arc_meta_max'], scale=BYTES_PER_GIB))
        if 'arc_meta_limit' in kstats:
            pr_desc(2, 'Metadata limit (GiB) =',
                    s_float(kstats['arc_meta_limit'], scale=BYTES_PER_GIB))

    pr_desc(1, 'Target size (GiB) =', s_float(kstats['c'], scale=BYTES_PER_GIB))
    pr_desc(2, 'Max target size limit (GiB) =',
            s_float(kstats['c_max'], scale=BYTES_PER_GIB))
    pr_desc(2, 'Min target size limit (GiB) =',
            s_float(kstats['c_min'], scale=BYTES_PER_GIB, fmt="%.3f"))

    pr_desc(1, 'MRU target size (GiB) =',
            s_float(kstats['p'], scale=BYTES_PER_GIB),
            s_pct(kstats['p'], scale=kstats['c'], of='target size'))

    mfu_target_size = float(kstats['c']) - float(kstats['p'])
    pr_desc(1, 'MFU target size (GiB) =',
            s_float(mfu_target_size, scale=BYTES_PER_GIB),
            s_pct(mfu_target_size, scale=kstats['c'], of='target size'))

    if 'compressed_size' in kstats and 'uncompressed_size' in kstats:
        pr_desc(1, 'Compressed ARC Sizes')
        pr_desc(2, 'Compressed size (GiB) =',
                s_float(kstats['compressed_size'], scale=BYTES_PER_GIB),
                s_pct(kstats['compressed_size'], scale=kstats['c'], of='target size'))
        pr_desc(2, 'Uncompressed size (GiB) =',
                s_float(kstats['uncompressed_size'], scale=BYTES_PER_GIB),
                s_pct(kstats['uncompressed_size'], scale=kstats['c'], of='target size'))
        arc_size_diff = kstats['uncompressed_size'] - kstats['compressed_size']
        pr_desc(2, 'Compressed ARC size difference (GiB) =',
                s_float(arc_size_diff, scale=BYTES_PER_GIB))
        arc_size_ratio = float(kstats['uncompressed_size']) / float(kstats['compressed_size'])
        pr_desc(2, 'Compressed ARC compression ratio =', s_float(arc_size_ratio, fmt='%.2f'))
        if kstats['uncompressed_size'] > kstats['c']:
            pr_analysis('Uncompressed ARC size > target size', status='good')
        if kstats['uncompressed_size'] > kstats['c_max']:
            pr_analysis('Uncompressed ARC size > max target size', status='excellent')

    # ARC efficiency analysis
    print()
    pr_desc(0, 'ARC Efficiency')
    current_arc_accesses = int(kstats['hits']) + int(kstats['misses'])
    pr_desc(1, 'Cache access total = ', s_int(current_arc_accesses))
    pr_desc(1, 'Cache hits =', s_int(kstats['hits']),
            s_pct(kstats['hits'], scale=current_arc_accesses, of='total accesses'))

    cache_analysis(kstats, 'Demand data', 'demand_data_hits', 'demand_data_misses',
                   'updating caching strategy')

    cache_analysis(kstats, 'Prefetch data', 'prefetch_data_hits', 'prefetch_data_misses',
                   'updating prefetch strategy')

    cache_analysis(kstats, 'Demand metadata', 'demand_metadata_hits', 'demand_metadata_misses',
                   'updating metadata cache strategy')

    cache_analysis(kstats, 'Prefetch metadata', 'prefetch_metadata_hits',
                   'prefetch_metadata_misses', 'updating metadata cache strategy')

    if 'mfu_hits' in kstats and 'mru_hits' in kstats:
        real_hits = int(kstats['mfu_hits']) + int(kstats['mru_hits'])
        pr_desc(1, 'Real hits (MFU + MRU) =',
                s_int(real_hits),
                s_pct(real_hits, scale=current_arc_accesses, of='total accesses'))
        pr_desc(2, 'MRU data hits =',
                s_int(kstats['mru_hits']),
                s_pct(kstats['mru_hits'], scale=real_hits, of='real hits'))
        pr_desc(2, 'MFU data hits =',
                s_int(kstats['mfu_hits']),
                s_pct(kstats['mfu_hits'], scale=real_hits, of='real hits'))
        if 'mru_ghost_hits' in kstats and 'mfu_ghost_hits' in kstats:
            pr_desc(2, 'MRU ghost hits =',
                    s_int(kstats['mru_ghost_hits']),
                    s_pct(kstats['mru_ghost_hits'], scale=real_hits, of='real hits'))
            pr_desc(2, 'MFU ghost data hits =',
                    s_int(kstats['mfu_ghost_hits']),
                    s_pct(kstats['mfu_ghost_hits'], scale=real_hits, of='real hits'))
            if (kstats['mru_ghost_hits'] + kstats['mfu_ghost_hits']) > (real_hits * GHOST_RATIO_OK):
                pr_analysis('ghost hits > {:.0f}% of real hits, '
                            'consider increasing ARC min size'.format(GHOST_RATIO_OK * 100),
                            status='info')
            else:
                pr_analysis('ghost hits < {:.0f}% of real hits'.format(GHOST_RATIO_OK * 100),
                            status='ok')

    # ARC eviction analysis
    if ('evict_l2_cached' in kstats and 'evict_l2_ineligible' in kstats
            and 'evict_l2_eligible' in kstats):
        print()
        pr_desc(0, 'ARC eviction analysis')
        evicted = (kstats['evict_l2_cached'] + kstats['evict_l2_ineligible'] +
                   kstats['evict_l2_eligible'])
        if evicted == 0:
            pr_analysis('Data has not been evicted from ARC')
        else:
            pr_desc(1, 'Total data evicted (GiB) =',
                    s_int(evicted, scale=BYTES_PER_GIB))
            pr_desc(2, 'Eligible for L2 (GiB) =',
                    s_int(kstats['evict_l2_eligible'], scale=BYTES_PER_GIB),
                    s_pct(kstats['evict_l2_eligible'], scale=evicted))
            pr_desc(2, 'Ineligible for L2 (GiB) =',
                    s_int(kstats['evict_l2_ineligible'], scale=BYTES_PER_GIB),
                    s_pct(kstats['evict_l2_ineligible'], scale=evicted))
            pr_desc(2, 'Already in L2 (GiB) = ',
                    s_int(kstats['evict_l2_cached'], scale=BYTES_PER_GIB),
                    s_pct(kstats['evict_l2_cached'], scale=evicted))
            if 'evict_mru' in kstats and 'evict_mfu' in kstats:
                pr_desc(2, 'Evicted from MRU (GiB) =',
                        s_int(kstats['evict_mru'], scale=BYTES_PER_GIB),
                        s_pct(kstats['evict_mru'], scale=evicted,
                              of='total data evicted'))
                pr_desc(2, 'Evicted from MFU (GiB) =',
                        s_int(kstats['evict_mfu'], scale=BYTES_PER_GIB),
                        s_pct(kstats['evict_mfu'], scale=evicted,
                              of='total data evicted'))

            if (float(kstats['evict_l2_eligible']) / evicted) > EVICTED_RATIO_OK:
                pr_analysis(
                    'Data evicted from ARC that is eligible for L2 > {:.0f}%, '
                    'consider adding cache device and increasing feed rate'.format(
                        EVICTED_RATIO_OK * 100),
                    status='info')


def l2arc_summary(kstats):
    """
    ZFS ARC usage summary

    :param kstats: parsed kstats
    :type kstats: dict
    """
    # Return now if no L2 activity
    if 'l2_feeds' not in kstats or kstats['l2_feeds'] == 0:
        pr_analysis('No L2ARC cache activity observed')
        return

    # L2ARC cache sizes and stats
    print()
    pr_desc(0, 'L2ARC cache statistics')
    if 'l2_hdr_size' in kstats:
        pr_desc(1, 'L2 header size (MiB) =',
                s_float(kstats['l2_hdr_size'], scale=BYTES_PER_MIB))

    if 'l2_size' in kstats:
        pr_desc(1, 'L2 size (GiB)=',
                s_float(kstats['l2_size'], scale=BYTES_PER_GIB))
        if 'l2_asize' in kstats:
            pr_desc(2, 'L2 allocated size (GiB)=',
                    s_float(kstats['l2_asize'], scale=BYTES_PER_GIB))
            pr_desc(2, 'L2 compression ratio =',
                    s_float(kstats['l2_size'], scale=kstats['l2_asize']))

    if 'l2_feeds' in kstats:
        pr_desc(1, 'L2 feeds =', s_int(kstats['l2_feeds']))

    if 'l2_writes_sent' in kstats:
        pr_desc(1, 'L2 writes sent =', s_int(kstats['l2_writes_sent']))
        if 'l2_writes_done' in kstats:
            pr_desc(2, 'L2 writes done =',
                    s_int(kstats['l2_writes_done']),
                    s_pct(kstats['l2_writes_done'],
                          scale=kstats['l2_writes_sent']))

    if 'l2_write_bytes' in kstats:
        pr_desc(1, 'L2 write bytes (GiB) =',
                s_float(kstats['l2_write_bytes'], scale=BYTES_PER_GIB))
        if 'l2_writes_sent' in kstats and kstats['l2_writes_sent'] != 0:
            pr_desc(2, 'Average L2 feed write size (KiB) = ',
                    s_float(float(kstats['l2_write_bytes']) / float(
                        kstats['l2_writes_sent']), scale=BYTES_PER_KIB))

    if 'l2_read_bytes' in kstats:
        pr_desc(1, 'L2 read bytes (GiB) =',
                s_float(kstats['l2_read_bytes'], scale=BYTES_PER_GIB))

    if 'l2_hits' in kstats and kstats['l2_hits'] != 0:
        pr_desc(2, 'Average L2 hit size (KiB) =',
                s_float(float(kstats['l2_read_bytes']) / float(kstats['l2_hits']),
                        scale=BYTES_PER_KIB))
        if ('l2_compress_failures' in kstats and 'l2_compress_successes' in kstats and
                'l2_compress_zeros' in kstats):
            current_l2_compresses = (kstats['l2_compress_successes'] +
                                     kstats['l2_compress_failures'])
            pr_desc(1, 'L2 compress successes =',
                    s_int(kstats['l2_compress_successes']),
                    s_pct(kstats['l2_compress_successes'], scale=current_l2_compresses))

    print()
    pr_desc(0, 'L2 cache efficiency')

    if 'l2_hits' in kstats and 'l2_misses' in kstats:
        l2_accesses = kstats['l2_hits'] + kstats['l2_misses']
        pr_desc(1, 'L2 cache total accesses =', s_int(l2_accesses))
        pr_desc(2, 'Hits =', s_int(kstats['l2_hits']),
                s_pct(kstats['l2_hits'], scale=l2_accesses, of='L2 cache total accesses'))
        # L2ARC cache efficiency is a simpler analysis than ARC
        if (float(kstats['l2_hits']) / l2_accesses) < CACHE_RATIO_OK:
            pr_analysis('L2 cache hit rate is low, consider efficacy of cache devices',
                        status='info')
        else:
            pr_analysis('L2 cache hit rate indicates the cache is useful', status='ok')

    print()
    pr_desc(0, 'L2 error statistics')
    if 'l2_abort_lowmem' in kstats:
        pr_desc(1, 'L2 abort due to lowmem =', s_int(kstats['l2_abort_lowmem']))
        if kstats['l2_abort_lowmem'] != 0:
            pr_analysis('L2ARC writes were aborted due to memory pressure, '
                        'consider adding RAM and increasing zfs_arc_min',
                        status='info')

    if 'l2_writes_error' in kstats:
        pr_desc(1, 'L2 write errors =', s_int(kstats['l2_writes_error']))
        if kstats['l2_writes_error'] != 0:
            pr_analysis('At some time in the past, writes to cache devices failed',
                        status='warning')

    if 'l2_io_error' in kstats:
        pr_desc(1, 'L2 I/O errors =', s_int(kstats['l2_io_error']))
        if kstats['l2_io_error'] != 0:
            pr_analysis('At some time in the past, reads from cache devices failed',
                        status='warning')

    if 'l2_cksum_bad' in kstats:
        pr_desc(1, 'L2 bad checksum =', s_int(kstats['l2_cksum_bad']))
        if kstats['l2_cksum_bad'] != 0:
            pr_analysis('At some time in the past, reads from cache devices had corruption',
                        status='warning')

    if 'l2_rw_clash' in kstats:
        pr_desc(1, 'L2 read/write clash =', s_int(kstats['l2_rw_clash']))
        if kstats['l2_rw_clash'] != 0:
            pr_analysis('L2ARC read/write clashes detected, consider changing l2arc_norw',
                        status='info')


def zpool_perf(kstats):
    """
    look at pool performance stats

    :param kstats: kstats collected
    :type kstats: dict
    """
    section_preamble(kstats, 'ZFS pool performance analysis')
    print('Analysis not yet implemented for Linux')
    # if 'zfs' not in kstat:
    #     pr_error('no zfs kstats found')
    #     return
    #
    # t = get_avg_ms_per_tick(kstat, uptime_nte)
    # print
    # 'Average clock tick = ' + s_float(t) + ' ms'
    # if t > 2:
    #     pr_analysis('old ZFS write throttle delays are very painful',
    #                 status='warning')
    #
    # f = False
    # for i in kstat['zfs']['0']:
    #     if 'class' in kstat['zfs']['0'][i]:
    #         if args.debug: print
    #         json.dumps(kstat['zfs']['0'][i], indent=2)
    #         if kstat['zfs']['0'][i]['class'] == 'disk':
    #             print
    #             '\nPool = ' + i
    #             class_disk_analysis(kstat['zfs']['0'][i], kstat,
    # type='zfspool')
    #             f = True
    # if not f:
    #     print
    #     'No pool performance information found'


def zfetchstat(kstats):
    """
    ZFS intelligent prefetcher performance stats

    :param kstats: kstats collected
    :type kstats: dict
    """
    section_preamble(kstats, 'ZFS prefetcher analysis')
    if 'hits' not in kstats or 'misses' not in kstats or 'max_streams' not in kstats:
        pr_error('cannot find zfetchstats')
        return

    accesses = int(kstats['hits']) + int(kstats['misses'])
    pr_desc(1, 'Total accesses = ', s_int(accesses))
    if accesses > 0:
        pr_desc(1, 'Hits = ', s_int(kstats['hits']), s_pct(kstats['hits'], scale=accesses))
        if (float(kstats['hits']) / accesses) < PREFETCH_RATIO_OK:
            pr_analysis('prefetch hit rate is low, consider tuning prefetcher')
        else:
            pr_analysis('prefetcher appears to be effective')

        # TODO: get proper calculation for elapsed time
        # if 'header' in kstats and type(kstats['header']) == list and len(kstats['header']) == 7:
        #     elapsed_time = float(kstats['header'][5]) / 1e9  # seconds
        # r = accesses / float(kstats['snaptime'])
        # c = 'high'
        # s = 'ok'
        # if r < 100: c = 'low'
        # e = 'effective'
        # if float(kstats['hits']) / accesses < 0.8:
        #     e = 'ineffective'
        #     s = 'info'
        # pr_analysis(
        #     'ZFS prefetcher is ' + e + ', confidence in analysis is ' + c,
        #     status=s)
    else:
        pr_analysis('ZFS prefetcher appears to be disabled')


def kmem_slab(kstats):
    """
    kernel memory usage analysis
    this analysis borrows from the techniques used in mdb's kmastat

    :param kstats: kstats collected
    :type kstats: dict
    """
    section_preamble(kstats, 'kernel kmem cache analysis')
    print('Analysis not yet implemented for Linux')
    # TODO: port to look at /proc/spl/kmem/slab
    # if not kstat_exists(kstat, ['unix', '0']):
    #     pr_error('no unix:0 kstats found')
    #     return
    #
    # c = []
    # total = 0
    # k = kstat['unix']['0']
    # for i in k:
    #     if 'class' in k[i] and k[i]['class'] == "kmem_cache":
    #         x = 0
    #         if 'buf_inuse' in k[i] and 'buf_size' in k[i]:
    #             x = int(k[i]['buf_inuse']) * int(k[i]['buf_size'])
    #         # if 'slab_create' in k[i] and 'slab_destroy' in k[i] and
    #         # 'slab_size' in k[i]:
    #         #   x = (int(k[i]['slab_create']) - int(k[i]['slab_destroy'])) *
    #         # int(k[i]['slab_size'])
    #         total += x
    #         d = k[i]
    #         d['name'] = i
    #         d['used'] = x
    #         c.append(d)
    #
    # print
    # 'Total size of kmem caches (GiB) = ' + s_float(total, scale=BYTES_PER_GIB)
    # l = sorted(c, key=itemgetter('used'), reverse=True)
    # n = int(args.top)
    # if len(l) < n or n < 1: n = len(l)
    # print
    # 'Top ' + str(n) + ' consumers of kmem_cache'
    # print
    # '\t\t%20s' % 'Consumer' + ' %10s' % 'Inuse(GiB)' + ' %6s' % 'kmem(%)'
    # ' %12s' % 'Buf_size(B)'
    # for i in range(n):
    #     print
    #     '\t\t%20s' % l[i]['name'],
    #     ' %10s' % s_float(l[i]['used'], scale=BYTES_PER_GIB),
    #     ' %6s' % s_float(l[i]['used'], scale=(total / 100)),
    #     ' %12s' % s_int(l[i]['buf_size'])
    #
    # print
    # 'kmem_move analysis'
    # c = []
    # for i in k:
    #     if ('class' in k[i] and k[i]['class'] == 'kmem_cache' and 'reap' in k[
    #         i] and
    #             'move_callbacks' in k[i] and k[i]['move_callbacks'] != '0'):
    #         d = k[i]
    #         d['name'] = i
    #         d['move_callbacks_int'] = int(k[i]['move_callbacks'])
    #         c.append(d)
    # l = sorted(c, key=itemgetter('move_callbacks_int'), reverse=True)
    # n = int(args.top)
    # if len(l) < n or n < 1: n = len(l)
    # if n == 0:
    #     pr_analysis('No kmem cache moves', status='info')
    # else:
    #     print
    #     'Top ' + str(n) + ' kmem caches moved'
    #     print
    #     '\t\t%20s' % 'Cache' + ' %15s' % 'Moves' + ' %15s' % 'Reaps'
    #     for i in range(n):
    #         print
    #         '\t\t%20s' % l[i]['name'],
    #         ' %15s' % s_int(l[i]['move_callbacks_int']) + ' %15s' % s_int(
    #             l[i]['reap'])


def cache_analysis(kstats, cache_name, hits_key, misses_key, consider):
    """
    print a cache hit rate analysis message

    :param kstats: kstats dict
    :type kstats: dict
    :param cache_name: name of cache under analysis
    :type cache_name: str
    :param hits_key: kstats key for hits
    :type hits_key: str
    :param misses_key: kstats key for misses
    :type misses_key: str
    :param consider: comment for consideration when ratio is < CACHE_RATIO_OK
    :type consider: str
    """
    if hits_key not in kstats and misses_key not in kstats:
        return
    accesses = kstats[hits_key] + kstats[misses_key]
    if accesses > 0:
        if 'hits' in kstats and 'misses' in kstats:
            total_accesses = kstats['hits'] + kstats['misses']
            pr_desc(2, '{} access = '.format(cache_name),
                    s_int(accesses), s_pct(accesses, scale=total_accesses, of='total accesses'))
        pr_desc(3, 'Hits =', s_int(kstats[hits_key]),
                s_pct(kstats[hits_key], scale=accesses, of='prefetch metadata accesses'))
        ratio = float(kstats[hits_key]) / float(accesses)
        if ratio > CACHE_RATIO_EXCELLENT:
            pr_analysis('{} cache hit rate > {:.0f}%'.format(
                cache_name, CACHE_RATIO_EXCELLENT * 100), status='excellent')
        elif ratio > CACHE_RATIO_GOOD:
            pr_analysis('{} cache hit rate > {:.0f}%'.format(
                cache_name, CACHE_RATIO_GOOD * 100), status='good')
        elif ratio > CACHE_RATIO_OK:
            pr_analysis('{} cache hit rate > {:.0f}%'.format(
                cache_name, CACHE_RATIO_OK * 100), status='ok')
        else:
            pr_analysis('{} cache hit rate < {:.0f}%, consider'.format(
                cache_name, CACHE_RATIO_OK * 100, consider), status='info')


# print formatting for backwards compatibility
# note: do simplistic divide-by-zero protection
def s_int(value, fmt='%d', scale=1):
    """
    convert int value to string and scale

    :param value: value
    :type value: int
    :param fmt: format string using locale.format() rules
    :type fmt: str
    :param scale: value is divided by scale
    :type scale: int
    :return:
    """
    if float(scale) == 0: scale = 1
    return locale.format(fmt, int(value) / int(scale), grouping=True)


def s_float(value, fmt='%.1f', scale=1):
    """
    convert float value to string and scale

    :param value: value
    :type value: float
    :param fmt: format string using locale.format() rules
    :type fmt: str
    :param scale: value is divided by scale
    :type scale: float
    :return:
    """
    if float(scale) == 0: scale = 1
    return locale.format(fmt, float(value) / float(scale), grouping=True)


def s_pct(value, fmt='%.0f', scale=1, parens=True, of=None):
    """
    print scaled value as a percent

    :param value: value
    :param fmt: floating point format
    :param scale: scaling factor, useful for "percent of ..."
    :type scale: float
    :param parens: if True, put value in parenthesis
    :type parens: bool
    :param of: percent of something
    :type of: str
    :return:
    """
    if float(scale) == 0.0: scale = 1
    s = locale.format(fmt, 100 * float(value) / float(scale),
                      grouping=True) + '%'
    if of:
        s += ' of ' + of
    if parens:
        s = ' (' + s + ')'
    return s


def pr_error(s):
    """
    print error string

    :param s: error string
    :type s: str
    """
    print('error: {}'.format(s))


def pr_desc(indent, desc, *values):
    """
    print a description line

    :param indent: number of tab indents
    :type indent: int
    :param desc: description string
    :type desc: str
    :param values: values printed with space separation
    :type values: str
    """
    print(indent * '\t', end='')
    print(desc, end='')
    for i in values:
        print(' {}'.format(i), end='')
    print()


def pr_analysis(s, status='ok'):
    """
    print analysis results string and status

    :param s: analysis results
    :type s: str
    :param status: severity indicator
    :type status: str
    """
    print('+ Analysis: status={}: {}'.format(status, s))


def read_kstats(filename, dirname='/proc/spl'):
    """
    read kstats from filename and convert to dict
    if the file cannot be read, return empty dict

    :param filename: name of file to read
    :type filename: str
    :param dirname: directory name
    :type dirname: str
    :return: kstats file contents
    :rtype: dict
    """
    res = {'filename': os.path.join(dirname, filename),
           'report_source': dirname}
    try:
        with open(res['filename']) as f:
            for line in f.readlines():
                s = line.split()
                if len(s) > 3:
                    # header line format is:
                    # ks_kid, ks_type, ks_flags, ks_ndata, ks_data_size, ks_crtime, ks_snaptime
                    res['header'] = s
                    continue
                if s[1] == 'type' or len(s) != 3:
                    continue
                if s[1] == '0' or s[1] == '7':
                    res[s[0]] = s[2]
                else:
                    res[s[0]] = int(s[2])
    except ValueError:
        pass
    except IOError as exc:
        pr_error('cannot read kstats: {}'.format(exc))
        return {}
    return res


def main():
    """
    the main event

    :return: exit code
    :rtype: int
    """
    analyzers = {
        'kmem': {'desc': 'ZFS Kernel slab memory allocations',
                 'run_me': False,
                 'funcs': [kmem_slab],
                 'files': ['kmem/slab']},
        'arc': {'desc': 'ZFS ARC',
                'run_me': True,
                'files': ['kstat/zfs/arcstats'],
                'funcs': [arc_summary, l2arc_summary]},
        'pool': {'desc': 'ZFS pool performance',
                 'run_me': False,
                 'funcs': [zpool_perf]},
        'prefetcher': {'desc': 'ZFS intelligent prefetcher',
                       'run_me': True,
                       'funcs': [zfetchstat],
                       'files': ['kstat/zfs/zfetchstats']},
    }

    if options.list:
        print('Available analyzers, + indicates those enabled by default:')
        for i in analyzers:
            d = ' '
            if analyzers[i].get('run_me', False):
                d = '+'
            print('  {:>15} {} {}'.format(i, d, analyzers[i].get('desc', '')))
        return 0

    if options.analysis:
        todo_list = options.analysis.split(',')
        work_todo = False
        for i in analyzers:
            if i in todo_list:
                analyzers[i]['run_me'] = True
                work_todo = True
            else:
                analyzers[i]['run_me'] = False
        for i in todo_list:
            if i not in analyzers:
                pr_error('ignoring unknown analyzer \"{}\"'.format(i))
        if not work_todo:
            pr_error('no valid analysis selected')
            return 1

    preamble()
    for directory in options.directory:
        for i in analyzers:
            if analyzers[i].get('run_me', False):
                kstats = {}
                for j in analyzers[i].get('files', []):
                    # assume no collisions in namespace for multiple files
                    kstats.update(read_kstats(j, dirname=directory))
                for j in analyzers[i].get('funcs', []):
                    j(kstats)

    return 0


if __name__ == '__main__':
    options = parse_args()
    locale.setlocale(locale.LC_ALL, '')
    try:
        res = main()
    except KeyboardInterrupt:
        sys.exit(0)
    sys.exit(res)
