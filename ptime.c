#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>

#define PTIME_FAILURE 2

static const char *signal_name(int sig)
{
    switch (sig) {
    case SIGHUP:  return "SIGHUP";
    case SIGINT:  return "SIGINT";
    case SIGQUIT: return "SIGQUIT";
    case SIGILL:  return "SIGILL";
    case SIGABRT: return "SIGABRT";
    case SIGFPE:  return "SIGFPE";
    case SIGKILL: return "SIGKILL";
    case SIGSEGV: return "SIGSEGV";
    case SIGPIPE: return "SIGPIPE";
    case SIGALRM: return "SIGALRM";
    case SIGTERM: return "SIGTERM";
    case SIGBUS:  return "SIGBUS";
    case SIGUSR1: return "SIGUSR1";
    case SIGUSR2: return "SIGUSR2";
    default:      return "unknown";
    }
}

static double ts_to_sec(const struct timespec *ts)
{
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1e9;
}

static double tv_to_sec(const struct timeval *tv)
{
    return (double)tv->tv_sec + (double)tv->tv_usec / 1e6;
}

static void print_report(FILE *out, char *const cmd[], pid_t pid, int status,
                         double wall, double user, double sys)
{
    fprintf(out, "=== ptime ===\n");

    fprintf(out, "%-7s : ", "command");
    for (int i = 0; cmd[i] != NULL; i++)
        fprintf(out, "%s%s", (i == 0) ? "" : " ", cmd[i]);
    fputc('\n', out);

    fprintf(out, "%-7s : %ld\n", "pid", (long)pid);

    if (WIFEXITED(status)) {
        fprintf(out, "%-7s : exited %d\n", "status", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        fprintf(out, "%-7s : signaled %d (%s)\n", "status", sig,
                signal_name(sig));
    } else {
        fprintf(out, "%-7s : unknown 0x%x\n", "status", (unsigned)status);
    }

    fprintf(out, "%-7s : %.3f s\n", "wall", wall);
    fprintf(out, "%-7s : %.3f s\n", "user", user);
    fprintf(out, "%-7s : %.3f s\n", "sys",  sys);
}

void inputError(int i, char *argv[]){
    if(argv[i] == NULL){
        fprintf(stderr, "ptime: Input error at %s%s", argv[i-1], ", use a int since empty...\n");
        exit(2);
    }
}
void readErrors(long var1, char *check, char *str){
    if(errno == ERANGE || errno == EINVAL){
            fprintf(stderr, "ptime: Input error OverFlow/UnderFlow at, %s\n", str);
            exit(2);
    }
    if(check == str || var1 < 1 || *check != '\0'){
            fprintf(stderr, "ptime: Input error number too small or contains characters not numbers at %s%s", str, ", use a int at reasonable size please...\n");
            exit(2);
    }
}
static volatile sig_atomic_t fired = 0;
static void on_alarm(int sig) { (void)sig; fired = 1; }
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "ptime: usage: ptime COMMAND [ARG]...\n");
        return PTIME_FAILURE;
    }
    char **cmd = &argv[1];
    char optionM = 'f';
    long time1 = __LONG_MAX__;
    long timesLoop = 1;
    int i = 1;
    while (argv[i] != NULL){
        if(strcmp(argv[i], "-m") == 0){
            optionM = 't';
            argv[i] = NULL;
        } else if(strcmp(argv[i], "-n") == 0){
            inputError(++i, argv);
            char *check;
            errno = 0;
            timesLoop = strtol(argv[i],&check,10);
            readErrors(timesLoop, check, argv[i-1]);
            argv[i-1] = NULL;
        } else if(strcmp(argv[i], "-t") == 0){
            inputError(++i, argv);
            char *check;
            errno = 0;
            time1 = strtol(argv[i],&check,10);
            readErrors(time1, check, argv[i-1]);
            argv[i-1] = NULL;
        }
        i++;
    }
    struct timespec t0, t1;
    struct rusage ru;
    struct rusage ru2 = {0};
    double avg = 0;
    double min = __DBL_MAX__;
    double max = 0;
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL); 
    pid_t child;
    int status = 0;
    int childStatus = 0;
    double saver;
    for(int l = 0; l < timesLoop; l++){
        fflush(NULL);
        int pipeErr[2] = {0};
        if(pipe(pipeErr) == -1){
            perror(strerror(errno));
            exit(2);
        }
        if(clock_gettime(CLOCK_MONOTONIC, &t0) == -1){
            perror(strerror(errno));
            exit(2);  
        }
        child = fork();
        if(child < 0){
            perror(strerror(errno));
            exit(2);
        }
        else if(child == 0){
            close(pipeErr[0]);
            execvp(cmd[0], cmd);
            int getErr = errno;
            fprintf(stderr, "ptime: cannot run '%s%s%s\n", cmd[0], "': ", strerror(getErr));
            write(pipeErr[1], &getErr, sizeof(int));
            close(pipeErr[1]);
            if(getErr == ENOENT){
                _exit(127);
            }else{
                _exit(126);
            }
        } else{
            close(pipeErr[1]);
            alarm(time1);
            while(waitpid(child, &status, 0) == -1){
                if(errno == EINTR){
                    if(fired){
                        kill(child, SIGKILL);
                        waitpid(child, &status, 0);
                        break;
                    }
                    continue;
                }else{
                    perror(strerror(errno));
                    exit(2);
                    break;
                }
            }
            if(clock_gettime(CLOCK_MONOTONIC, &t1) == -1){
                perror(strerror(errno));
                exit(2);
            }
            if(getrusage(RUSAGE_CHILDREN, &ru) == -1){
                perror(strerror(errno));
                exit(2);
            }
            read(pipeErr[0],&childStatus, sizeof(int));
            close(pipeErr[0]);
        }
        saver = ts_to_sec(&t1) - ts_to_sec(&t0);
        print_report(stderr, cmd, child, status,
                 saver,
                 tv_to_sec(&ru.ru_utime) - tv_to_sec(&ru2.ru_utime),
                 tv_to_sec(&ru.ru_stime) - tv_to_sec(&ru2.ru_stime));
        fprintf(stderr, "%-7s : exited %d\n", "errno", childStatus);
        if(optionM == 't'){
            fprintf(stderr, "%-7s : %ld KB\n", "maxrss", ru.ru_maxrss);
        }
        if(time1 != __LONG_MAX__){
            (fired == 0) ? fprintf(stderr, "%-7s : %s\n", "Timer", "Pass") : fprintf(stderr, "%-7s : %s\n", "Timer", "Failed");
            fired = 0;
        }
        avg += saver;
        if(max < saver){
            max = saver;
        }
        if(min > saver){
            min = saver;
        }
        childStatus = 0;
        ru2 = ru;
}
    if(timesLoop > 1){
        fprintf(stderr, "=== ptime avg ===\n");
        fprintf(stderr, "%-7s : %.3f s\n", "wall avg", (avg/timesLoop));
        fprintf(stderr, "%-7s : %.3f s\n", "wall max", max);
        fprintf(stderr, "%-7s : %.3f s\n", "wall min", min);
    }
    if(WIFEXITED(status)){
        return WEXITSTATUS(status);
    }else if(WIFSIGNALED(status)){
        return (128 + WTERMSIG(status));
    }
    return PTIME_FAILURE;
}
