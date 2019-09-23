§§#!/bin/bash
# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Roman Bendt, Thomas Riedmaier, Pascal Eckmann
§§
# Check command line parameters

NODE=$(uname -n)
DO_CHECK_DB=FALSE

for ARGUMENT in "$@"
do

	KEY=$(echo $ARGUMENT | cut -f1 -d=)
	VALUE=$(echo $ARGUMENT | cut -f2 -d=)   

	case "$KEY" in
§§		NODE)
§§			NODE=${VALUE}
§§			;;
§§		DO_CHECK_DB)
§§			DO_CHECK_DB=${VALUE}
§§			;;
		*)   
	esac    


done

§§# BUG: on our docker host, not all /dev/sd* are found by df
§§
§§if [[ "$(echo $0 | sed 's/.*\///')" == "fm.sh" ]] ; then
§§	exec 1> /dev/null
§§	exec 2>&1
§§fi
§§
§§if [[ "$(whoami)" != "root" ]] ; then
§§#if [[ $EUID -ne 0 ]] ; then
§§	echo "must be root"
§§	exit 1
§§fi
§§
§§
§§APPENDIX=/tmp/fluffmon.tar
§§PLUGINDIR=/tmp/fluffmonplugins
§§CLIENT=/tmp/mqttcli-$(uname -m)
§§HOST=mon.fluffi
§§TOPIC="FLUFFI/state/${NODE}/"
§§ALERT="FLUFFI/alert/${NODE}/"
§§PUB="$CLIENT pub -s --host $HOST -q 2 -t ${TOPIC}"
§§
§§COUNTER=$((0))
§§
§§ARCHIVE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' $0)
§§tail -n+$ARCHIVE $0 > $APPENDIX
§§if [[ ! -f "$APPENDIX" ]] ; then
§§	echo "unpacking failed"
§§	exit 2
§§fi
§§tar -xf $APPENDIX -C /tmp
§§
§§echo "starting fluffmon on $NODE"
§§uname -a | ${PUB}"register"
§§while true ; do
§§
§§	# check disk space
§§	VOLMS=$( df -PTh|egrep -iw "ext4|ext3|xfs|gfs|gfs2|btrfs|overlay"|sort -k6n|awk '!seen[$1]++' )
§§	while read -r N ; do
§§		VOL=$(echo $N | awk '{print $1}' | sed 's/\///g')
§§		USE=$(echo $N | awk '{print $6}' | sed 's/%//g')
§§		echo "$USE" | ${PUB}"vol:$VOL"
§§	done <<< "$VOLMS"
§§
	# check database
	if [ "$DO_CHECK_DB" = TRUE ]  ; then 
§§		if [[ $((COUNTER % 5)) -eq 0 ]] ; then
§§			TBLS=$(mysql -h db.fluffi -u fluffi_gm -pfluffi_gm -e 'SELECT table_schema AS "Database", ROUND(SUM(data_length + index_length) / 1024 / 1024, 2) AS "Size (MB)" FROM information_schema.TABLES GROUP BY table_schema;' 2>/dev/null | tail -n+2 | head -n -3)
§§			while read -r N ; do
§§				DB=$(echo $N | awk '{print $1}')
§§				USED=$(echo $N | awk '{print $2}')
§§				echo "$USED" | ${PUB}"db:$DB"
§§			done <<< "$TBLS"
§§		fi
§§	fi
§§
§§	# run plugins
§§	if [[ -d "$PLUGINDIR" ]] ; then
§§		for PLUG in $(find $PLUGINDIR -type f -perm -100) ; do
§§			API="$($PLUG "API")"
§§			if [[ "$API" != "FLUFFIMONITORING" ]] ; then
§§				continue
§§			fi
§§			TYPE="$($PLUG "TYPE")"
§§			NAME="$($PLUG "NAME")"
§§			OUT="$($PLUG "RUN")"
§§			if [[ "$TYPE" == "ALERT" ]] ; then
§§				$CLIENT pub --host $HOST -t ${ALERT}${NAME} -m "$OUT"
§§			else
§§				$CLIENT pub --host $HOST -t ${TOPIC}${NAME} -m "$OUT"
§§			fi
§§		done
§§	fi
§§	sleep 5
§§	COUNTER=$((COUNTER + 1))
§§done
§§uname -a | ${PUB}"unregister"
§§
§§exit 0
§§
§§__ARCHIVE_BELOW__
