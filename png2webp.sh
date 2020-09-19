#!/bin/sh
"${0%.sh}" -bv -- "$@"
ret=$?
if $ret
then
  read -rsn1 -p'Press any key to continue'
  exit $ret
fi
