#!/bin/sh
"${0%.sh}" -bv -- "$@"
ret=$?
if [ $ret -ne 0 ]
then
  echo -n 'Press enter to continue'
  read -r duh
fi
exit $ret
