#!/bin/bash

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

while true; do
read -p "Do you really want do delete nscan?(y/n) " yn

case $yn in 
	[yY] ) echo Uninstalling...;
        make clean &> /dev/null
        err=$(sudo rm /usr/bin/nscan 2>&1 > /dev/null)
        if [[ -n $err ]]
        then
            make clean
            echo "Nscan has already been deleted!"
            exit 0
        fi
        for number in $(seq ${_start} ${_end})
        do
            sleep 0.05
            ProgressBar ${number} ${_end}
        done
        echo -e "\nDone!"
        exit;;
	[nN] ) echo exiting...;
		exit;;
	* ) echo invalid response;;
esac
done