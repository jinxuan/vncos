
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#define TRUE 1
#define FALSE !TRUE

#define BUFFER_MAX_LENGTH 50
static char userInput[3];
static char buffer[BUFFER_MAX_LENGTH];
static int bufferChars = 0;

static char *commandArgv[5];
static int commandArgc = 0;

static int fd;


#define FOREGROUND 'F'
#define BACKGROUND 'B'
#define SUSPENDED 'S'
#define WAITING_INPUT 'W'


#define STDIN 1
#define STDOUT 2


#define BY_THREAD_ID 1
#define BY_JOB_ID 2
#define BY_JOB_STATUS 3

static int numActiveJobs = 0;

typedef struct job {
        int id;
        char *name;
        int tid;
//        pid_t pgid;
        int status;
//        char *descriptor;
        struct job *next;
} t_job;

static t_job* jobsList = NULL;



//static pid_t MSH_PID;
//static pid_t MSH_PGID;
static int MSH_TERMINAL, MSH_IS_INTERACTIVE;
static struct termios MSH_TMODES;

void init_command(void);

t_job* insert_job(char* name, int status, int jobtid,int *jobid);

t_job* del_job(t_job* job);

t_job* get_job(int searchValue, int searchParameter);

void print_jobs(void);

void welcome_screen(void);

void shell_prompt(void);
void handle_usercommand(void);
void get_textline(void);
void populate_command(void);


int check_builtin_commands(int s, int execute_mode);

void launch_job(char *command, void (*function)(void *), int i_data,int executionMode);

void kill_job(int jobId);

