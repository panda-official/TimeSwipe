#include "pidfile.hpp"
#include "defs.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>

PidFile::PidFile(const std::string& name)
: fname (name) {
#if NOT_RPI
    fname = std::string("./") + name + std::string(".pid");
#else
    fname = std::string("/var/run/") + name + std::string(".pid");
#endif
}

bool PidFile::Lock(std::string& err) {
    if (locked) return true;
    fd = open(fname.c_str(), O_CREAT | O_RDWR, 0600);
    if (fd < 0) {
        err = std::string("lock open failed: ") + std::string(strerror(errno));
        return false;
    }
    int rc = flock(fd, LOCK_EX | LOCK_NB);
    if(rc) {
        if(EWOULDBLOCK == errno) {
            err = "another instance running";
        } else {
            err = std::string("flock failed: ") + std::string(strerror(errno));
        }
        unlock();
        return false;
    }
    char buf[32];

    int rd = read(fd, buf, sizeof(buf)-1);
    buf[rd]=0;

    char proc_name[80];
    snprintf(proc_name, sizeof(proc_name), "/proc/%s/exe", buf);
    if( access( proc_name, F_OK ) != -1 ) {
        err = "process exists with pid " + std::string(buf);
        unlock();
        return false;
    }

    snprintf(buf, sizeof(buf), "%d", getpid());
    rd = write(fd, buf, strlen(buf));
    locked = true;

    return true;
}

void PidFile::unlock() {
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    if (locked) {
        unlink(fname.c_str());
        locked = false;
    }
}

PidFile::~PidFile() {
    unlock();
}

#ifdef TEST_PIDFILE
int main()
{
    PidFile pidlock("timeswipe");
    std::string err;
    if (!pidlock.Lock(err)) {
        printf("lock failed: %s\n", err.c_str());
        return -1;
    }
    printf("locked\n");
    getchar();
    pidlock.Unlock();
    printf("unlocked\n");
}
#endif
