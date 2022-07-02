#!/usr/bin/env sh
"${0%webptopng.sh}png2webp" -rv -- "$@"
ret=$?
if [ $ret -ne 0 ]
then
  echo -n 'Press enter to continue'
  read -r duh
fi
exit $ret
