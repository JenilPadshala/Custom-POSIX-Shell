#include "../include/pinfo.h"
#include "../include/prompt.h" 
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <libproc.h>
#include <sys/proc_info.h>
#include <sys/sysctl.h>

void execute_pinfo(char** args, int arg_count) {
    pid_t target_pid;

    // determine target PID
    // if no arg, target is current shell pid
    if (arg_count == 1) {
        target_pid = getpid(); 
    } 
    // if one arg, convert arg to pid 
    else if (arg_count == 2) {
        char* endptr = nullptr;
        target_pid = static_cast<pid_t>(std::strtol(args[1], &endptr, 10));
        if (*endptr != '\0' || target_pid <= 0) {
            std::fprintf(stderr, "pinfo: invalid PID '%s'\n", args[1]);
            return;
        }
    } else {
        std::fprintf(stderr, "pinfo: too many arguments\n");
        return;
    }

    //NOTE: I used libproc.h from here on out; this command won't work on linux machine.

    // fetch process status & process group info
    struct proc_bsdinfo bsd_info;
    int bsd_res = proc_pidinfo(target_pid, PROC_PIDTBSDINFO, 0, &bsd_info, sizeof(bsd_info));
    // if process not found or access denied, print error and return
    if (bsd_res <= 0) {
        std::fprintf(stderr, "pinfo: process with PID %d not found or access denied\n", target_pid);
        return;
    }

    char status_char = 'S';
    // status definitions for reference: 1 = SIDL, 2 = SRUN, 3 = SSLEEP, 4 = SSTOP, 5 = SZOMB
    switch (bsd_info.pbi_status) {
        case 2: status_char = 'R'; break;
        case 3: status_char = 'S'; break;
        case 4: status_char = 'T'; break;
        case 5: status_char = 'Z'; break;
        default: status_char = 'S'; break;
    }

    // check if the process is in the foreground; also check cross-terminal process group
    bool is_foreground = false;
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, target_pid};
    struct kinfo_proc kp;
    size_t kp_size = sizeof(kp);

    if(sysctl(mib, 4, &kp, &kp_size, nullptr, 0) == 0 && kp_size > 0){
        if(kp.kp_eproc.e_tpgid != -1 && kp.kp_eproc.e_pgid == kp.kp_eproc.e_tpgid){
            is_foreground = true;
        }
    }

    char status_str[4];
    if(is_foreground){
        std::snprintf(status_str, sizeof(status_str), "%c+", status_char);
    } else {
        std::snprintf(status_str, sizeof(status_str), "%c", status_char);
    }

    // fetch virtual memory usage
    struct proc_taskinfo task_info;
    uint64_t virtual_memory = 0;
    int task_res = proc_pidinfo(target_pid, PROC_PIDTASKINFO, 0, &task_info, sizeof(task_info));
    if (task_res > 0) {
        virtual_memory = task_info.pti_virtual_size;
    }

    // fetch executable path
    char exec_path[PROC_PIDPATHINFO_MAXSIZE];
    int path_res = proc_pidpath(target_pid, exec_path, sizeof(exec_path));
    if (path_res <= 0) {
        std::strncpy(exec_path, "Unavailable", sizeof(exec_path));
    }

    // format relative to shell_home if applicable
    char formatted_path[PROC_PIDPATHINFO_MAXSIZE];
    size_t home_len = std::strlen(shell_home);
    if (std::strncmp(exec_path, shell_home, home_len) == 0) {
        formatted_path[0] = '~';
        std::strncpy(formatted_path + 1, exec_path + home_len, sizeof(formatted_path) - 2);
        formatted_path[sizeof(formatted_path) - 1] = '\0';
    } else {
        std::strncpy(formatted_path, exec_path, sizeof(formatted_path) - 1);
        formatted_path[sizeof(formatted_path) - 1] = '\0';
    }

    // display the process information
    std::printf("pid -- %d\n", target_pid);
    std::printf("Process Status -- {%s}\n", status_str);
    std::printf("memory -- %llu {Virtual Memory}\n", virtual_memory);
    std::printf("Executable Path -- %s\n", formatted_path);
}