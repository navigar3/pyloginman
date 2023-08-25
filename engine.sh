#!/bin/sh

SendMsg()
{
  printf "%3d\x00%s" "${#1}" "${1}"
}

ReadMsg()
{
  read -N 3 PSZ
  read -N $PSZ VAR
  echo -n "$VAR"
}

SendMsg "READY"

while true; do

  MSG=$( ReadMsg )
  
  SendMsg "ACK"
  
  if [ "$MSG" = "QUIT" ]; then
    break
  fi
  
  echo "[ENGINE]" "$MSG" >&2
  
done

echo "[ENGINE]" "Uscita" >&2

exit 0
