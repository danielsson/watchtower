
#include <stdio.h>
#include <sys/inotify.h>
#include <limits.h>
#include <time.h>

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

time_t lastRun = 0;

static void handleModification(struct inotify_event *i, char *script)
{
    if (i->mask & IN_MODIFY) {
        
        time_t currentTime = time(NULL);
        if (currentTime - lastRun > 1) {
            printf("[%s]Ran script %s", ctime(&currentTime), script);
            printf("\n");
            
            system(script);
            lastRun = currentTime;
        }
    }
}


int main(int argc, char *argv[])
{
    int inotifyFd, wd, j;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p;
    struct inotify_event *event;
    
    if (argc < 3 || strcmp(argv[1], "--help") == 0) {
        printf("%s: run a script when files changes \n", argv[0]);
        printf("%s script pathname...\n", argv[0]);
        return -1;
    }
    
    inotifyFd = inotify_init();                 /* Create inotify instance */
    if (inotifyFd == -1) {
        printf("inotify_init");
        return -1;
    }
    
    /* For each command-line argument, add a watch for all events */
    
    for (j = 2; j < argc; j++) {
        wd = inotify_add_watch(inotifyFd, argv[j], IN_MODIFY);
        if (wd == -1) {
            printf("inotify_add_watch");
            return -1;
        }
        
        printf("Watching %s using wd %d\n", argv[j], wd);
    }
    
    for (;;) {
        numRead = read(inotifyFd, buf, BUF_LEN);
        if (numRead == 0) {
            printf("read() from inotify fd returned 0!");
            return -1;
        }
        
        if (numRead == -1)
            return -1;
        
        for (p = buf; p < buf + numRead; ) {
            event = (struct inotify_event *) p;
            handleModification(event, argv[1]);
            
            p += sizeof(struct inotify_event) + event->len;
        }
    }
    
    return 0;
}