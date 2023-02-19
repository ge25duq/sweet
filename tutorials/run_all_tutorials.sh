#! /bin/bash


# Stop at first error
set -e

RR=run_recursive_tutorials.sh
RT=run_tutorial.sh

THISDIR=$(pwd)

for DIR in */ ; do
	echo "$THISDIR/$DIR"
	cd "$DIR"

	if [ -x  "./$RR" ]; then
		./$RR
	elif [ -x  "./$RT" ]; then 
		./$RT
	else
		echo "Don't know how to handle directory '$THISDIR/$DIR'"
		exit 1
	fi

	cd "${THISDIR}"
done

