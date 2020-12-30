#! /bin/bash

cd "$(dirname $0)"

for i in $(ls -1 -d ??_*/); do
	i=$(basename "$i")
	echo_info_hline
	echo_info "Runing compile tests for $i"
	echo_info_hline
	./$i/test.* > output_"${i/\//}"".out" || exit 1
done
