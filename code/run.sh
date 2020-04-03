mkdir ../log
logfile=$(date "+%Y-%m-%d-%H:%M:%S")
./server 8888 3 | tee "../log/"${logfile}".log"
