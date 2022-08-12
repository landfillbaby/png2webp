#!/usr/bin/env sh
PATH="$(dirname -- "$(readlink -f -- "$0")"):$PATH" png2webp -v -- "$@"
ret=$?
if [ $ret -ne 0 ]
then
  echo -n 'Press enter to continue'
  read -r duh
fi
exit $ret
