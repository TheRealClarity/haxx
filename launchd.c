/* THIS IS VERY DANGEROUS PLEASE DON'T USE THIS (only tested once) */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <spawn.h>
#include <string.h>
#include <IOKit/IOKitLib.h>

#define LAUNCHD "/sbin/launchd.real"
#define HAXX "/sbin/haxx"

static const char __unused fakelaunchd[] = "fakelaunchd";
char *real_argv[] = {"LAUNCHD_HAXX", NULL};

int main(int argc, char *argv[], char *envp[])
{
    if (getpid() != 1)
    {
        fprintf(stderr, "fakelaunchd cannot be run directly.\n");
        exit(1);
    }
    int fd_console = open("/dev/console", O_RDWR, 0);
    dup2(fd_console, 0);
    dup2(fd_console, 1);
    dup2(fd_console, 2);
    for (uint8_t i = 0; i < 10; i++)
    {
        printf("============ WE ARE PID 1 ============\n");
    }
    close(fd_console);
    close(0);
    close(1);
    close(2);

    io_registry_entry_t entry = IORegistryEntryFromPath(MACH_PORT_NULL, "IODeviceTree:/options");
    if (entry == 0)
    {
        execve(LAUNCHD, real_argv, envp);
        fprintf(stderr, "cannot execute %s!g: %s... bailing\n", LAUNCHD, strerror(errno));
        exit(1);
    }

    CFStringRef cfNvramVar = IORegistryEntryCreateCFProperty(entry, CFSTR("boot-args"), kCFAllocatorDefault, 0);
    if (cfNvramVar == NULL)
    {
        execve(LAUNCHD, real_argv, envp);
        fprintf(stderr, "cannot execute %s!g: %s... bailing\n", LAUNCHD, strerror(errno));
        exit(1);
    }

    const char *nvramVar = CFStringGetCStringPtr(cfNvramVar, kCFStringEncodingUTF8);

    if (nvramVar != NULL)
    {
        if (strstr(nvramVar, "no_untether") != NULL)
        {
            execve(LAUNCHD, real_argv, envp);
            fprintf(stderr, "cannot execute %s!g: %s... bailing\n", LAUNCHD, strerror(errno));
            exit(1);
        }
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        execve(HAXX, real_argv, envp);
        fprintf(stderr, "cannot execute %s!g: %s... bailing\n", HAXX, strerror(errno));
        exit(1);
    }
    else
    {
        execve(LAUNCHD, argv, envp);
        fprintf(stderr, "cannot execute %s!g: %s... spinning\n", LAUNCHD, strerror(errno));
        while (1)
        {
            sleep(__INT_MAX__);
        }
    }
    exit(42);
}