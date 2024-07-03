#!/bin/sh

PID=$(echo $$)
CGROUP_DIR=/sys/fs/cgroup/perf_event/test/
if [[ ! -d $CGROUP_DIR ]]
then
    sudo mkdir -p $CGROUP_DIR
fi
echo $PID | sudo tee $CGROUP_DIR/cgroup.procs

btop &
# sudo ../profiler ${PID} 1
# sudo chown yuan:yuan report.html
# mv report.html ../
