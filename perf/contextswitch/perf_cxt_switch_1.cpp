#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>


#include <map>
#include <unordered_set>
#include <queue>
using namespace std;


#define error(msg) do { perror(msg); exit(1); } while(0)
#define MAXN  128

static long perf_event_open(struct perf_event_attr *perf_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, perf_event, pid, cpu, group_fd, flags);
}
static void *addr = NULL;
static int fd = -1;
static long long psize;
map<int, pair<void*, long long>> res;
void int_exit(int s) {
    for (auto x: res) {
        auto y = x.second;
        void* addr = y.first;
        munmap(addr, (1+MAXN)*psize);
        close(x.first);
    }
    res.clear();
}

static unsigned long long cx_count=0, g_stime=0, g_total=0;
static priority_queue<unsigned long long, vector<unsigned long long>, greater<unsigned long long>> events;

// static int ssss[4]={0};
int process_event(char *base, unsigned long long size, unsigned long long offset) {
    struct perf_event_header* p = NULL;
    int pid, cpuid, xpid;
    unsigned long long time;
    offset%=size;
    // assuming the header would fit within size
    p = (struct perf_event_header*) (base+offset);
    offset+=sizeof(*p); if (offset>=size) offset-=size;
    if (p->type == PERF_RECORD_SAMPLE) {
        return p->size;
    } else if (p->type == PERF_RECORD_SWITCH) {
        return p->size;
    } else if (p->type == PERF_RECORD_SWITCH_CPU_WIDE) {
        xpid = *((int *)(base+offset));
        offset+=8; if (offset>=size) offset-=size;
    } else {
        return p->size;
    }
    pid = *((int *)(base+offset));  offset+=8; if (offset>=size) offset-=size;
    time = *((unsigned long long *)(base+offset));  offset+=8; if (offset>=size) offset-=size;
    events.push(time);
    time/=1000000000;
    while(!events.empty()) {
        unsigned long long t = events.top();
        t/=1000000000;
        if (time<t||time-t<3) break;
        cx_count++;
        if (g_stime==0) g_stime=t;
        if (g_stime<t) {
            g_total+=cx_count;
            printf("%10lld>: %10lld/s   total: %-10lld\n", g_stime, cx_count, g_total); fflush(stdout);
            cx_count = 0;
            g_stime=t;
        }
        events.pop();
    }
    return p->size;
}

#define MAXCPU 1024
struct pollfd polls[MAXCPU];
int main(int argc, char *argv[]) {
    if (argc != 2) { printf("need pid\n"); return 1; }
    int pid = atoi(argv[1]); if (pid<=0) { printf("invalid pid %s\n", argv[1]); return 1; }
    // find cgroup
    char xb[256], xb2[256];
    int i, j, k;
    sprintf(xb, "/proc/%d/cgroup", pid);
    FILE* fp = fopen(xb, "r");
    if (fp==NULL) error("fail to open cgroup file");
    char *p;
    xb2[0]=0;
    int cgroup_name_len=0;
    while(1) {
        p = fgets(xb, sizeof(xb), fp); if (p==NULL) break;
        i=0; while(p[i]&&p[i]!=':') i++; if (p[i]==0) continue; 
        if (strstr(p, "perf_event")) {
            i++; while(p[i]!=':'&&p[i]) i++;  if (p[i]!=':') continue; i++;
            j=i; while(p[j]!='\r'&&p[j]!='\n'&&p[j]!=0) j++; p[j]=0;
            sprintf(xb2, "/sys/fs/cgroup/perf_event%s", p+i);
            cgroup_name_len=j-i;
            break;
        } else if (p[i+1]==':') {
            i+=2; j=i; while(p[j]!='\r'&&p[j]!='\n'&&p[j]!=0) j++; p[j]=0;
            sprintf(xb2, "/sys/fs/cgroup/%s", p+i);
            cgroup_name_len=j-i;
        }
    }
    fclose(fp);
    if (xb2[0]==0) error("no proper cgroup found\n");
    if (cgroup_name_len<2) {
        printf("cgroup %s seems to be root, not allowed\n", xb2);
        return -1;
    }
    printf("try to use cgroup %s\n", xb2);
    int cgroup_id = open(xb2, O_CLOEXEC);
    if (cgroup_id<=0) { perror("error open cgroup dir"); return 1; }
    // start perf event
    psize = sysconf(_SC_PAGE_SIZE); // getpagesize();
    int cpu_num = sysconf(_SC_NPROCESSORS_ONLN);
	struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.type = PERF_TYPE_SOFTWARE;
    attr.size = sizeof(attr);
    attr.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
    // attr.config = PERF_COUNT_SW_CPU_MIGRATIONS;
    attr.sample_period = 1;
    attr.wakeup_events = 32;
    attr.sample_type = PERF_SAMPLE_TID|PERF_SAMPLE_TIME|PERF_SAMPLE_CPU;
    attr.context_switch = 1;
    attr.sample_id_all = 1;
    for (i=0, k=0; i<cpu_num&&i<MAXCPU; i++) {
        printf("attaching cpu %d\n", i);
        fd = perf_event_open(&attr, cgroup_id, i, -1, PERF_FLAG_FD_CLOEXEC|PERF_FLAG_PID_CGROUP);
        if (fd<0) { perror("fail to open perf event"); continue; }
        addr = mmap(NULL, (1+MAXN)*psize, PROT_READ, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) { perror("mmap failed"); close(fd); continue; }
        res[fd] = make_pair(addr, 0);
        polls[k].fd = fd;
        polls[k].events = POLLIN;
        polls[k].revents = 0;
        k++;
    }
    if (k==0) { printf("no cpu event attached at all\n"); return 1; }

	signal(SIGINT, int_exit);
	signal(SIGTERM, int_exit);

    unsigned long long head;
    struct perf_event_mmap_page *mp;
    while (poll(polls, k, -1)>0) {
        for (i=0; i<k; i++) {
            if ((polls[i].revents&POLLIN)==0) continue;
            fd = polls[i].fd;
            addr = res[fd].first;
            mp = (struct perf_event_mmap_page *)addr;
            head = res[fd].second;
            if (head==mp->data_head) continue;
            ioctl(fd, PERF_EVENT_IOC_PAUSE_OUTPUT, 1);
            head = mp->data_head-((mp->data_head-head)%mp->data_size);
            while(head<mp->data_head) head+=process_event((char*)addr+mp->data_offset, mp->data_size, head);
            ioctl(fd, PERF_EVENT_IOC_PAUSE_OUTPUT, 0);
            res[fd].second = mp->data_head;
        }
    }

    int_exit(0);
    return 0;
}
