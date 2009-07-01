#include <stdio.h>
#include <sys/unistd.h>
#include <stdint.h>
#include "definitions.h"	//sh defination
#include "hypectrl.h"		//sh jobs interact with xen hypervisor
#include "shapp.h"		//sh jobs

#define MAXLINE 4096

void handle_usercommand(void)
{
        if(check_builtin_commands(0,FOREGROUND) == -1) 
		printf("There is no such command!\n");
}

void print_help(void)
{
	printf("The command list :\n");
	printf("help(h)		get command list\n");
	printf("quit(q)		quit from mini-os\n");
	printf("echo		print string to mini-os console\n");
	printf("wait num	wait for num seconds\n");
	printf("bg job		executing the job at background, for example :bg wait 10\n");
	printf("lsjob		list all active jobs\n");
	printf("lsthread	list all running threads\n");
	printf("lsxenstore	list local domain's xenstore, or list specified path, ex:lsxenstore /local/domain/0\n");
	printf("rdtsc		read tsc time stamp\n");
}

int check_builtin_commands(int s, int execute_mode)
{
	if(strcmp("h", commandArgv[s])==0 || strcmp("help", commandArgv[s])==0)
	{
		print_help();
		return 0;
	}

        if (strcmp("q", commandArgv[s])==0 || strcmp("quit", commandArgv[s])==0) 
	{
                exit(EXIT_SUCCESS);
        }
        
	if(strcmp("echo", commandArgv[s])==0 )
	{
		int i=1;

		do
		{
			if(commandArgv[i])
				printf("%s ",commandArgv[i]);
			else
			{
				printf("\n");
				break;
			}
			i++;
		}while(1);

		return 0;
	}

	if(strcmp("wait", commandArgv[s])==0 )
	{
		launch_job(commandArgv[s],(void *)sh_wait,s+1,execute_mode);
		return 0;
	}

	if(strcmp("bg", commandArgv[s])==0 )
	{
		if(commandArgv[s+1] == NULL || s>0)
			return 0;

		if(check_builtin_commands(s+1,BACKGROUND)!= -1)
			return 0;
		else	
			return -1;	
	}

	if(strcmp("lsjob", commandArgv[s])==0 )
        {
		update_jobs();
		print_jobs();
		return 0;
	}

	if(strcmp("lsthread", commandArgv[s])==0 )
	{
		print_runqueue();
		return 0;
	}

	if(strcmp("lsxenstore", commandArgv[s])==0 )
	{
		ls_xenstore(commandArgv,s,commandArgc);
		return 0;
	}
	
	if(strcmp("rdtsc", commandArgv[s])==0 )
	{
		read_tsc();
		return 0;
	}	

	if (strcmp("kill", commandArgv[0]) == 0)
        {
                if (commandArgv[1] == NULL)
                        return 0;
                kill_job(atoi(commandArgv[1]));
                return 1;
        }

        return -1; 
}

void launch_job(char *command, void (*function)(void *),int i_data,int executionMode)
{
	struct thread *th=NULL;
	int jobid;
	t_job* job;
	int in=i_data;

	th = create_thread(command,function,(void *)in);
	
        jobsList = insert_job(command, (int) executionMode, th->tid ,&jobid);

	job =get_job(jobid, BY_JOB_ID);

	if (executionMode == FOREGROUND)
	{
		while(is_alive(th))
			usleep(2000);
		kill_job(jobid);
	}

	if (executionMode == BACKGROUND)
		printf("[%d] %s     executing at background.\n", job->id, job->name);
}

void kill_job(int jobId)
{
        t_job *job = get_job(jobId, BY_JOB_ID);
        jobsList = del_job(job);
}

int main(int argc, char **argv, char **envp)
{
	fd=alloc_fd(FTYPE_CONSOLE);
        welcome_screen();
        shell_prompt();

        while (TRUE) {
                read(fd,userInput,1);
                switch (*userInput) {
                case '\r':
			printf("\n");
                        shell_prompt();
                        break;
                default:
                        get_textline();
                        handle_usercommand();
                        shell_prompt();
                        break;
                }
        }
        printf("\n");
        return 0;
}

void get_textline()
{
        init_command();
        while ((*userInput != '\r') && (bufferChars < BUFFER_MAX_LENGTH)) 
        {
		if(*userInput != 0x7f)
                	buffer[bufferChars++] = *userInput;
		else
		{
			if(bufferChars <= 0)
				bufferChars=0;
			else
			{
				printf("%c%c%c",'\b',' ','\b');
				fflush(stdout);
				bufferChars--;
			}
		}
			
                read(fd,userInput,1);
        }
        buffer[bufferChars] = 0x00;
	printf("\n");
        populate_command();
}

void populate_command()
{
        char* bufferPointer;
        	bufferPointer = strtok(buffer, " ");
        while (bufferPointer != NULL) {
                commandArgv[commandArgc] = bufferPointer;
                bufferPointer = strtok(NULL, " ");
                commandArgc++;
        }
}

void init_command()
{
        while (commandArgc != 0) {
                commandArgv[commandArgc] = NULL;
                commandArgc--;
        }
        bufferChars = 0;
}


t_job* insert_job(char* name, int status, int jobtid, int *jobid)
{
        t_job *newJob = malloc(sizeof(t_job));
        usleep(10000);

        newJob->name = (char*) malloc(sizeof(name));
        newJob->name = strcpy(newJob->name, name);
        newJob->status = status;
	newJob->tid = jobtid;
        newJob->next = NULL;

        if (jobsList == NULL) {
                numActiveJobs++;
                newJob->id = numActiveJobs;
		*jobid = newJob->id;
                return newJob;
        } else {
                t_job *auxNode = jobsList;
                while (auxNode->next != NULL) {
                        auxNode = auxNode->next;
                }
                newJob->id = auxNode->id + 1;
                auxNode->next = newJob;
                numActiveJobs++;
		*jobid= newJob->id;
                return jobsList;
        }
}

t_job* del_job(t_job* job)
{
        t_job* currentJob;
        t_job* beforeCurrentJob;

        usleep(10000);
        if (jobsList == NULL)
                return NULL;

        currentJob = jobsList->next;
        beforeCurrentJob = jobsList;

        if (beforeCurrentJob->id == job->id) {

                beforeCurrentJob = beforeCurrentJob->next;
                numActiveJobs--;
		free(job);
		jobsList = beforeCurrentJob;
                return currentJob;
        }

        while (currentJob != NULL) {
                if (currentJob->id == job->id) {
                        numActiveJobs--;
                        beforeCurrentJob->next = currentJob->next;
                }
                beforeCurrentJob = currentJob;
                currentJob = currentJob->next;
		break;
        }
	
	free(job);
        return jobsList;
}

t_job* get_job(int searchValue, int searchParameter)
{
        t_job* job = jobsList;
        usleep(10000);

        switch (searchParameter) {
        case BY_JOB_ID:
                while (job != NULL) {
                        if (job->id == searchValue)
                                return job;
                        else
                                job = job->next;
                }
                break;
        case BY_JOB_STATUS:
                while (job != NULL) {
                        if (job->status == searchValue)
                                return job;
                        else
                                job = job->next;
                }
                break;
	case BY_THREAD_ID:
		while(job != NULL) {
			if (job->tid == searchValue)
				return job;
			else
				job = job->next;
		}
        default:
                return NULL;
                break;
        }
        return NULL;
}

void update_jobs()
{
        t_job *job = jobsList;
	t_job *pjob=NULL;
	struct thread *th;
	
	while(job != NULL)
	{
		th= get_thread_byid(job->tid);
		if(th == NULL)
		{
			//printf("kill job->tid(%d)\n",job->tid);
			if(job->next == NULL)
			{
				jobsList=NULL;
				return ;
			}
			jobsList = del_job(job);
			job = pjob;
		}
		else 
		{
			pjob=job;
			job = job->next;
		}
	}	
}

void print_jobs()
{
        t_job* job = jobsList;

        printf("\nActive jobs:\n");
        printf("---------------------------------------------------------------------------\n");
        printf("| %7s | %7s | %15s | %6s |\n", "job no.", "thread id","name", "status");
        printf("---------------------------------------------------------------------------\n");
        if (job == NULL) {
                printf("| %s %62s |\n", "No Jobs.", "");
        } else {
                while (job != NULL) {
                        printf("| %7d | %7d   | %15s | %6c |\n", job->id, job->tid,job->name,job->status);
                        job = job->next;
                }
        }
        printf("---------------------------------------------------------------------------\n");
}

void welcome_screen()

{
        printf("\n-------------------------------------------------\n");
        printf("\tWelcome to MINI-shell  \n");
	printf("      Type 'h' for command list\n");
        printf("-------------------------------------------------\n");
        printf("\n\n");
}

void shell_prompt()
{
        printf("mini-sh :> ");
	fflush(stdout);
}
