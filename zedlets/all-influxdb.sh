#!/usr/bin/env bash
#
# Log the zevent to an influxdb or telegraf listener using influxdb line protocol
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
[[ -f "${ZED_ZEDLET_DIR}/zed.rc" ]] && source "${ZED_ZEDLET_DIR}/zed.rc"
source "${ZED_ZEDLET_DIR}/zed-functions.sh"

zed_exit_if_ignoring_this_event
zed_check_cmd nc || exit 3 # internal error if nc not found

function escape_tag {
    echo $1 | sed -e 's/\"/\\\"/g;s/,/\\,/g;s/ /\\ /g'
}

# variables likely to need to be edited per local setup
INFLUXDB_HOST=${ZED_INFLUXDB_HOST:=172.16.28.1}
INFLUXDB_PORT=${ZED_INFLUXDB_PORT:=8094}
NODENAME=${ZED_NODENAME:=$(uname -n)}

POOL=\"$(escape_tag ${ZEVENT_POOL:=unknown})\"
GUID=\"$(escape_tag ${ZEVENT_POOL_GUID:=0})\"
CLASS=$(escape_tag ${ZEVENT_CLASS:=unknown})
EID=$(escape_tag ${ZEVENT_EID:=0})
TIMESTAMP=$(echo ${ZEVENT_TIME} | awk '{printf("%s%09d", $1, $2)}')

TAGS="zfs_events,class=${CLASS},host=${NODENAME}"
METRICS="eid=${EID},guid=${GUID},pool=${POOL}"

echo "${TAGS} ${METRICS} ${TIMESTAMP}" | nc -N -q 10 ${INFLUXDB_HOST} ${INFLUXDB_PORT}

exit 0
