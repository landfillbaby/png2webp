#!/bin/sh
PATH="$(dirname -- "$(readlink -f -- "$0")"):$PATH" png2webp -v -- "$@"
ret=$?
if [ $ret -ne 0 ]
then
	echo Press enter to continue
	read -r x
fi
exit $ret
