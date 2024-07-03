#!/usr/bin

CGROUP_DIR=/sys/fs/cgroup/perf_event/test/
if [[ ! -d $CGROUP_DIR ]]
then
    sudo mkdir -p $CGROUP_DIR
fi

PROGPATH=/home/yuan/Benchmarks/ascylib-vanilla/bin/
PROG="lf-ll_harris -n 2 -i 16000000"

$PROGPATH/${PROG} &

PID=$!
echo $PID | sudo tee $CGROUP_DIR/cgroup.procs

sudo ../profiler $PID 0
sudo chown yuan:yuan report.html
mv report.html ../
