process_runner () {
    for i in $(seq 1 $ITERATORNUMBER)
    do
        INDEX=$(($i - 1))
        echo "${TASKSARRAY[$INDEX]}" | ./asset_conv - $ITERATORPROCESSNUMBER 2> /dev/null &
    done
    wait
}


ITERATORNUMBER="${1:-4}"
echo "Time running asset_conv with 1 thread:"
time ./asset_conv tasks.csv 1 2> /dev/null

echo "Time running asset_conv with $ITERATORNUMBER threads:"
time ./asset_conv tasks.csv $ITERATORNUMBER 2> /dev/null

NUMBEROFLINES=$(wc -l < tasks.csv)
if [[ $(($NUMBEROFLINES % $ITERATORNUMBER)) = 0 ]]
then
    LINESDIV=$(($NUMBEROFLINES / $ITERATORNUMBER))
    for i in $(seq 1 $ITERATORNUMBER)
    do
        TASKSARRAY+=("$(head -$(($i * $LINESDIV)) tasks.csv | tail +$(($i * $LINESDIV - $LINESDIV + 1)))")
    done
    ITERATORPROCESSNUMBER=1
    echo "Time running asset_conv with $ITERATORNUMBER processes and 1 thread per process:"
    time process_runner
    ITERATORPROCESSNUMBER="${2:-4}"
    echo "Time running asset_conv with $ITERATORNUMBER processes and $ITERATORPROCESSNUMBER threads per process:"
    time process_runner
else
    echo "tasks.csv needs to be divisable by $ITERATORNUMBER"
fi