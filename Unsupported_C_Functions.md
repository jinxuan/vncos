在Mini-OS中调用以下函数会报错或使系统crash

### FS ###
link,umask,relink

### 以后版本中支持 ###
chdir

### 动态库 ###
dlopen,dlsym,dlerror,dlclose

### 信号 ###
sigemptyset,sigfillset,sigaddset,sigdelset,sigismember,sigprocmask,sigaction,sigsetjmp,sigaltstack,kill

### C调用 ###
pipe,fork,execv,execve,waitpid,wait,lockf,sysconf,tcsetattr,tcgetattr,poll

### net/if.h ###
if\_nametoindex,if\_indextoname,if\_freenameindex

### 参考 ###
XEN\_ROOT/extras/mini-os/lib/sys.c