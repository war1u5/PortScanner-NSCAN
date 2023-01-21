#!/bin/bash

make clean
sudo rm /usr/bin/nscan 2> /dev/null

make nscan 2> /dev/null
cale=`pwd`
newcale="$cale/nscan"
sudo ln -s $newcale /usr/bin/nscan 2> /dev/null

function ProgressBar {
    let _progress=(${1}*100/${2}*100)/100
    let _done=(${_progress}*4)/10
    let _left=40-$_done
    
    _fill=$(printf "%${_done}s")
    _empty=$(printf "%${_left}s")

    printf "\rProgress : [${_fill// /#}${_empty// /-}] ${_progress}%%"
}

_start=1
_end=100

for number in $(seq ${_start} ${_end})
do
    sleep 0.05
    ProgressBar ${number} ${_end}
done
echo -e "\nDone!"
exit 0