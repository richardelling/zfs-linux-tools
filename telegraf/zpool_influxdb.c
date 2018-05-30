/*
 * Gather top-level ZFS pool and resilver/scan statistics and print using
 * influxdb line protocol
 * usage: [pool_name]
 *
 * To integrate into telegraf, use the inputs.exec plugin
 *
 * NOTE: libzfs is an unstable interface. YMMV.
 * For Linux ompile with: gcc -lzfs -lnvpair zpool_influxdb.c -o zpool_influxdb
 *
 * Copyright 2018 Richard Elling
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/fs/zfs.h>
#include <time.h>
#include <libzfs.h>
#include <string.h>

#define	POOL_MEASUREMENT	"zpool_stats"
#define SCAN_MEASUREMENT	"zpool_scan_stats"

/*
 * in cases where ZFS is installed, but not the ZFS dev environment, copy in
 * the needed definitions from libzfs_impl.h
 */
#ifndef _LIBZFS_IMPL_H
struct zpool_handle {
	libzfs_handle_t *zpool_hdl;
	zpool_handle_t *zpool_next;
	char zpool_name[ZFS_MAX_DATASET_NAME_LEN];
	int zpool_state;
	size_t zpool_config_size;
	nvlist_t *zpool_config;
	nvlist_t *zpool_old_config;
	nvlist_t *zpool_props;
	diskaddr_t zpool_start_block;
};
#endif

/*
 * influxdb line protocol rules for escaping are important because the
 * zpool name can include characters that need to be escaped
 *
 * caller is responsible for freeing result
 */
char *
escape_string(char *s) {
	char *c, *d;
	char *t = (char *)malloc(ZFS_MAX_DATASET_NAME_LEN << 1);
	if (t == NULL) {
		fprintf(stderr, "error: cannot allocate memory\n");
		exit (1);
	}

	for (c = s, d = t; *c != '\0'; c++, d++) {
		switch (*c) {
			case ' ':
			case ',':
			case '=':
			case '\\':
				*d++ = '\\';
			default:
				*d = *c;
		}
	}
	*d = '\0';
	return (t);
}

void
print_scan_status(pool_scan_stat_t *ps, const char *pool_name) {
	int64_t elapsed;
	uint64_t examined, pass_exam, paused_time, paused_ts, rate;
	uint64_t remaining_time;
	double pct_done;
	char *state[DSS_NUM_STATES] = {"none", "scanning", "finished", "canceled"};
	char *func = "unknown_function";

	/*
	 * ignore if there are no stats or state is bogus
	 */
	if (ps == NULL || ps->pss_state >= DSS_NUM_STATES ||
		ps->pss_func >= POOL_SCAN_FUNCS)
		return;

	switch (ps->pss_func) {
		case POOL_SCAN_NONE:
			func = "none_requested";
			break;
		case POOL_SCAN_SCRUB:
			func = "scrub";
			break;
		case POOL_SCAN_RESILVER:
			func = "resilver";
			break;
#ifdef POOL_SCAN_REBUILD
		case POOL_SCAN_REBUILD:
				func = "rebuild";
				break;
#endif
		default:
			func = "scan";
	}

	/* overall progress */
	examined = ps->pss_examined ? ps->pss_examined : 1;
	pct_done = 0.0;
	if (ps->pss_to_examine > 0)
		pct_done = 100.0 * examined / ps->pss_to_examine;

#ifdef EZFS_SCRUB_PAUSED
	paused_ts = ps->pss_pass_scrub_pause;
			paused_time = ps->pss_pass_scrub_spent_paused;
#else
	paused_ts = 0;
	paused_time = 0;
#endif

	/* calculations for this pass */
	if (ps->pss_state == DSS_SCANNING) {
		elapsed = time(NULL) - ps->pss_pass_start - paused_time;
		elapsed = (elapsed > 0) ? elapsed : 1;
		pass_exam = ps->pss_pass_exam ? ps->pss_pass_exam : 1;
		rate = pass_exam / elapsed;
		rate = (rate > 0) ? rate : 1;
		remaining_time = ps->pss_to_examine - examined / rate;
	} else {
		elapsed = ps->pss_end_time - ps->pss_pass_start - paused_time;
		elapsed = (elapsed > 0) ? elapsed : 1;
		pass_exam = ps->pss_pass_exam ? ps->pss_pass_exam : 1;
		rate = pass_exam / elapsed;
		remaining_time = 0;
	}
	rate = rate ? rate : 1;

	/* influxdb line protocol format: "tags metrics timestamp" */
	(void) printf("%s,function=%s,pool=%s,state=%s ",
				  SCAN_MEASUREMENT, func, pool_name, state[ps->pss_state]);
	(void) printf("end_ts=%lui,errors=%lui,examined=%lui,function=\"%s\","
				  "pass_examined=%lui,pause_ts=%lui,paused_t=%lui,"
				  "pct_done=%.2f,processed=%lui,rate=%lui,"
				  "remaining_t=%lui,start_ts=%lui,state=\"%s\","
				  "to_examine=%lui,to_process=%lui ",
				  ps->pss_end_time,
				  ps->pss_errors,
				  examined,
				  func,
				  pass_exam,
				  paused_ts,
				  paused_time,
				  pct_done,
				  ps->pss_processed,
				  rate,
				  remaining_time,
				  ps->pss_start_time,
				  state[ps->pss_state],
				  ps->pss_to_examine,
				  ps->pss_to_process
	);
	(void) printf("%lu000000000\n", time(NULL));
}


int
print_stats(zpool_handle_t *zhp, void *data) {
	uint_t c;
	boolean_t missing;
	nvlist_t *nv, *nv_ex, *config, *nvroot;
	vdev_stat_t *vs;
	uint64_t *lat_array;
	char *pool_name;
	pool_scan_stat_t *ps = NULL;

	/* if not this pool return quickly */
	if (data && strncmp(data, zhp->zpool_name, ZFS_MAX_DATASET_NAME_LEN) != 0)
		return (0);

	if (zpool_refresh_stats(zhp, &missing) != 0)
		return (1);

	config = zpool_get_config(zhp, NULL);

	if (nvlist_lookup_nvlist(config,
							 ZPOOL_CONFIG_VDEV_TREE, &nv) != 0) {
		return (2);
	}

	if (nvlist_lookup_uint64_array(nv,
								   ZPOOL_CONFIG_VDEV_STATS,
								   (uint64_t **)&vs, &c) != 0) {
		return (3);
	}
	if (nvlist_lookup_nvlist(config, ZPOOL_CONFIG_VDEV_TREE, &nvroot) != 0) {
		return (4);
	}
	(void) nvlist_lookup_uint64_array(nvroot,
									  ZPOOL_CONFIG_SCAN_STATS,
									  (uint64_t **)&ps, &c);

	pool_name = escape_string(zhp->zpool_name);
	(void) printf("%s,name=%s,state=%s ", POOL_MEASUREMENT, pool_name,
		   zpool_state_to_name(vs->vs_state, vs->vs_aux));
	(void) printf("alloc=%lui,free=%lui,size=%lui,state=\"%s\","
			   "read_bytes=%lui,read_errors=%lui,read_ops=%lui,"
			   "write_bytes=%lui,write_errors=%lui,write_ops=%lui,"
			   "checksum_errors=%lui,fragmentation=%lui",
				  vs->vs_alloc,
				  vs->vs_space - vs->vs_alloc,
				  vs->vs_space,
				  zpool_state_to_name(vs->vs_state, vs->vs_aux),
				  vs->vs_bytes[ZIO_TYPE_READ],
				  vs->vs_read_errors,
				  vs->vs_bytes[ZIO_TYPE_READ],
				  vs->vs_bytes[ZIO_TYPE_WRITE],
				  vs->vs_write_errors,
				  vs->vs_ops[ZIO_TYPE_WRITE],
				  vs->vs_checksum_errors,
				  vs->vs_fragmentation);
	(void) printf(" %lu000000000\n", time(NULL));

	print_scan_status(ps, pool_name);

	free(pool_name);
	return (0);
}

int
main(int argc, char *argv[]) {
	libzfs_handle_t *g_zfs;
	g_zfs = libzfs_init();
	if (argc > 1) {
		return(zpool_iter(g_zfs, print_stats, argv[1]));
	} else {
		return(zpool_iter(g_zfs, print_stats, NULL));
	}
}

