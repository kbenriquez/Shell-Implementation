//Author: Kyle Marcus Enriquez

#include "sfish.h"
#include "debug.h"


sigset_t mask, prev_mask, block_all, unblock_all;
char message[50] = "";
char child_message[130] = "\nChild with PID ";
char child_message2[22] = " has died. It spent ";
char child_message3[40] = " milliseconds utilizing the CPU\n";
char *usr2_message = "\nWell that was easy.\n";
char *shell_prompt;
struct sigaction action;



char toChar[50];

char child_PID[10];
int info_pid;
double info_utime;
double info_stime;

volatile sig_atomic_t alarm_triggered = 0;
volatile sig_atomic_t child_terminated = 0;

char *itoa(int num, char *str){
	int i = 0;

	while(num != 0){
		int left = num % 10;
		//printf("left = %d\n", left);
		str[i] = left + '0';
		num = num / 10;
		i++;
	}

	char ret[i];

	int j = i;
	int k = 0;
	i--;
	while(i >= 0){
		ret[k] = str[i];
		k++;
		i--;
		//printf("k = %c", ret[k]);
	}
	ret[j] = '\0';
	char *ptr = ret;
	return ptr;
}

void sigusr2_handler(){
	write(1, usr2_message, strlen(usr2_message));
	write(1, shell_prompt, strlen(shell_prompt));
}


void alarm_handler(int sig){
	write(1, message, strlen(message));
	write(1, shell_prompt, strlen(shell_prompt));
	/*alarm message*/
    message[0] = '\0';
}

int temp_child_status;
void sigchld_handler(int sig, siginfo_t *info, void *ignore){
	if(sigprocmask(SIG_BLOCK, &block_all, &unblock_all)==-1){
		fprintf(stderr, "ERROR: Could not block signal(s) in child handler\n");
	}

	info_pid = info->si_pid;
	info_utime = info->si_utime;
	info_stime = info->si_stime;
	child_terminated = 1;
	char *temp = itoa(info_pid, toChar);
	strcat(child_message, temp);

	double cpu = (info_stime + info_utime) * 10;
	toChar[0] = '\0';
	if(cpu == 0){
		temp = NULL;
		temp = "0";
	}
	else{
		temp = itoa(cpu, toChar);
	}
	strcat(child_message, child_message2);
	strcat(child_message, temp);
	strcat(child_message, child_message3);

	write(1, child_message, strlen(child_message));
	child_message[16] = '\0';

	//write(1, shell_prompt, strlen(shell_prompt));
	if(sigprocmask(SIG_SETMASK, &unblock_all, NULL)==-1){
		fprintf(stderr, "ERROR: Could not unblock signal(s) in child handler\n");
	}
}




int rogue;
void singlePipe(int pipefd[], char *firstArg, char **firstProg, char **secondProg, char **thirdProg, char **paths, int numPipes){

	/*Create the pipes*/
    if(pipe(pipefd) == -1){
        fprintf(stderr, "ERROR: Could not creat pipe\n");
    }

    struct stat checkExists;

    int j = 0;
    char *firstArg2 = NULL;
    char *firstArg3 = NULL;

    /*Get the path of the program*/
    while(paths[j] != NULL){
    	char *tempor = malloc(800);
        strcat(tempor, paths[j]);
        strcat(tempor, "/");
        strcat(tempor, secondProg[0]);

        /*Check if program exists*/
        if(stat(tempor, &checkExists) == 0){
        	firstArg2 = tempor;
        	break;
        }
        j++;
    }

    /*If double piping, get third arguments array*/
    if(numPipes == 2){
    	j = 0;
    	while(paths[j] != NULL){
    		char *tempora = malloc(800);
        	strcat(tempora, paths[j]);
        	strcat(tempora, "/");
        	strcat(tempora, thirdProg[0]);

        	/*Cehck if program exists*/
        	if(stat(tempora, &checkExists) == 0){
        		firstArg3 = tempora;
        		break;
        	}
        	j++;
   	 	}
    };

    int child;
    child = fork();

    switch(child){
    	case -1:
    		fprintf(stderr, "ERROR: Could not execute fork()\n");
    		break;
    	case 0:
    		/*THIS DOES NOT DIE*/
    		rogue = getpid();

    		/*If not Double Piping*/
    		if(numPipes != 2){
    			close(pipefd[1]);
    		}
    		else{
    			/*Get the arguments of second pipe, check existence, dup2 the pipeends, fork then execute*/
    			int pipeEnds[2];
    			int child2;
    			child2 = fork();
    			pipe(pipeEnds);

    			if(child2 == 0){
    				dup2(pipeEnds[0], 0);
    				close(pipeEnds[1]);
    				if(execv(firstArg3, thirdProg)){
    					fprintf(stderr, "ERROR: Could not execute third program\n");
    					break;
    				}
    			}
    			else{
    				//printf("hello?\n");
    				dup2(pipeEnds[1], 1);
    				printf("___executing second arg\n");
    				if(execv(firstArg2, secondProg)){
    					fprintf(stderr, "ERROR: Could not execute second program\n");
    					break;
    				}
    			}
    		}

    		dup2(pipefd[0], 0);
    		if(execv(firstArg2, secondProg)){
    			fprintf(stderr, "ERROR: Could not execute program\n");
    		}
    		exit(0);
    		break;

    	default:
    			dup2(pipefd[1], 1);
    			close(pipefd[0]);
    			if(execv(firstArg, firstProg)){
    				fprintf(stderr, "ERROR: Could not execute program\n");
    			}
    			break;

    			wait(NULL);

    }
}



int main(int argc, char const *argv[], char* envp[]){
    /* DO NOT MODIFY THIS. If you do you will get a ZERO. */
    rl_catch_signals = 0;



    /*Blocking SIGTSP*/
    if((sigemptyset(&mask)) == -1){
    	fprintf(stderr, "ERROR: sigemptyset(&mask) could not execute\n");
	}
	if((sigaddset(&mask, SIGTSTP)) == -1){
		fprintf(stderr, "ERROR: sigaddset(&mask, SIGTSTP) could not execute\n");
	}
	if((sigprocmask(SIG_BLOCK, &mask, &prev_mask)) == -1){
		fprintf(stderr, "ERROR: sigprocmask could not execute, SIGTSTP not blocked\n");
	}


	if(sigfillset(&block_all) == -1){
    	fprintf(stderr, "ERROR: sigfillset(&block_all) could not execute\n");
    }


	/*Install SIGALRM handler*/
	signal(SIGALRM, alarm_handler);

	/*Install SIGUSR2 handler*/
	signal(SIGUSR2, sigusr2_handler);

	/*Handle SIGCHLD*/
	struct sigaction action;
	action.sa_sigaction = sigchld_handler;
	if(sigfillset(&action.sa_mask)==-1){
		fprintf(stderr, "ERROR: sigfillset(&action.sa_mask) could not execute\n");
	}
	action.sa_flags = SA_SIGINFO;
	if(sigaction(SIGCHLD, &action, NULL) == -1){
		fprintf(stderr, "ERROR: sigaction(SIGCHLD, &action, NULL) could not execute\n");
	}

    char *cmd;



    /*Get list of paths in PATH environment*/
    char **paths = (char**)malloc(30 * sizeof(char*));
    int pathAmt = 0;
    char *temp = getenv("PATH");
    if(temp == NULL){
    	fprintf(stderr, "ERROR: getenv(PATH) could not execute\n");
    }
    char *path = strtok(temp, ":");
    paths[pathAmt++] = path;
    while((path = strtok(NULL, ":")) != NULL){
        paths[pathAmt] = path;
        pathAmt++;
    }




    /*set prompt*/
    char prompt[1000] = {'k','b','e','n','r','i','q','u','e','z',' ',':',' ','\0'};
    char cwd[1000];
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, " $");
    strcat(prompt, cwd);

    /*keep track of previous directory for cd -*/
    char prevDir[1000];
    getcwd(prevDir, sizeof(prevDir));

    shell_prompt = prompt;
    while((cmd = readline(prompt)) != NULL) {

    	/*child message*/


    	/*exit*/
        if (strcmp(cmd, "exit")==0)
            break;

        /*help*/
        else if(strcmp(cmd, "help") == 0){
        	printf("---SFISH SHELL---\n\n");
        	printf("USAGE: exit\n\tExits the shell\n");
        	printf("USAGE: pwd\n\tPrints to stdout the current directory\n");
        	printf("USAGE: cd [directory]\n\tChange the current directory to <directory>\n");
        }

        /*cd*/
        else if(strncmp(cmd, "cd", 2) == 0){

        	/*Get command and flag*/
        	char *cd = strtok(cmd, " ");
        	char *flag = strtok(NULL, " ");

        	/*Check if command is valid*/
        	if(strcmp(cd, "cd") != 0){
        		fprintf(stderr, "ERROR: Unknown command %s\n", cd);
        		continue;
        	}
        	if(strtok(NULL, " ") != NULL){
        		fprintf(stderr, "ERROR: Too many arguments for cd\n");
        		continue;
        	}

        	/*If the command was only 'cd'*/
        	if(flag == NULL){
        		getcwd(prevDir, sizeof(prevDir));
        		if(chdir(getenv("HOME")) == 0){
        			char po[100];
        			getcwd(po, sizeof(po));
        			if(setenv("PWD", po, 1) == -1){
        				fprintf(stderr, "ERROR: setenv(PWD, po, 1) could not execute\n");
        			}
        		}
        		else{
        			fprintf(stderr, "ERROR: Cannot go to HOME directory\n");
        			continue;
        		}
        	}
        	else{

        		char curDir[1000];
        		getcwd(curDir,sizeof(curDir));

        		if(strcmp(cd, "cd") != 0){
        			fprintf(stderr, "ERROR: Unknown command %s\n", cd);
        			continue;
        		}

        		/*cd -*/
        		if(strcmp(flag, "-") == 0){
        			char temp[1000];
        			getcwd(temp, sizeof(temp));
        			if(chdir(prevDir) == 0){
        				char po[100];
        				getcwd(po, sizeof(po));
        				if(setenv("PWD", po, 1) == -1){
        					fprintf(stderr, "ERROR: setenv(PWD, po, 1) could not execute\n");
        				}
        			}
        			else{
        				fprintf(stderr, "ERROR: Could not change directories\n");
        			}
        			strncpy(prevDir, temp, 1000);
        		}

        		/*cd .*/
        		else if(strcmp(flag, ".") == 0){
        			char same[200];
        			getcwd(same, sizeof(same));
        			chdir(same);
        		}

        		/*cd ..*/
        		else if(strcmp(flag, "..") == 0){
        			char *ptr = strrchr(curDir, '/');

        			*ptr = '\0';
        			getcwd(prevDir, sizeof(prevDir));
        			if(chdir(curDir) == 0){
        				char po[100];
        				getcwd(po, sizeof(po));
        				if(setenv("PWD", po, 1) == -1){
        					fprintf(stderr, "ERROR: setenv(PWD, po, 1) could not execute\n");
        				}
        			}
        			else{
        				fprintf(stderr, "ERROR: 'cd ..' returned NULL\n");
        			}
        		}

        		/*ERROR HANDLING*/
        		else{
        			strcat(curDir, "/");
        			strcat(curDir, flag);
        			DIR* dir = opendir(curDir);
        			if(dir){
        				if(chdir(curDir) == 0){}
        				else{
        					fprintf(stderr, "ERROR: Unknown flag %s for command cd\n", flag);
        				}
        			}
        			else if(ENOENT == errno){
        				fprintf(stderr, "ERROR: Directory does not exist\n");
        			}
        			else{
        				fprintf(stderr, "ERROR: Cannot change directories\n");
        			}
        		}
        	}
        }

        /*pwd*/
        else if(strcmp(cmd, "pwd") == 0){
        	int pid, child_status;
        	char curDir[1000];


        	/*fork and let child print out current directory then exit*/
        	if((pid = fork()) == 0){
        		if(getcwd(curDir, sizeof(curDir)) != NULL){
        			printf("%s\n", curDir);
        		}
        		else{
        			fprintf(stderr, "ERROR: pwd returned NULL!\n");
        		}
        		exit(0);
        	}
        	else{
        		wait(&child_status);
        	}
        }

        /*alarm*/
        else if(strncmp(cmd, "alarm", 5) == 0){

        	/*Get 'alarm' argument*/
        	char *alrm = strtok(cmd, " ");

        	/*Get following argument after alarm*/
        	char *arg = strtok(NULL, " ");

        	/*Temporary variable to check for excess input*/
        	char *temp = strtok(NULL, " ");
        	int valid = 1;

        	/*Check if all characters of the arg variable is a number*/
        	if(arg != NULL){
        		int i = 0;
        		while(arg[i] != '\0'){
        			if(isdigit(arg[i])){
        				i++;
        			}
        			else{
        				valid = 0;
        				break;
        			}
        		}
        	}

        	/*CHECK FOR ERRORS IN CALL*/
        	/*alarm is followed by some characters*/
        	if(strcmp(alrm, "alarm") != 0){
        		fprintf(stderr, "ERROR: unknown command %s\n", alrm);
        	}

        	/*alarm does not have an argument*/
        	else if(arg == NULL){
        		fprintf(stderr, "ERROR: alarm needs an integer argument\n");
        	}

        	/*alarm has too many arguments*/
        	else if(temp != NULL){
        		fprintf(stderr, "ERROR: Too many arguments for alarm\n");
        	}

        	/*the argument is 0*/
        	else if(atoi(arg) == 0){
        		fprintf(stderr, "ERROR: argument cannot be 0\n");
        	}

        	/*the argument is not an integer*/
        	else if(valid == 0){
        		fprintf(stderr, "ERROR: argument given to alarm was not an integer\n");
        	}

        	/*alarm and its arguments are valid*/
        	else {
        		int seconds = atoi(arg);
        		message[0] = '\0';
        		strcat(message, "\nYour ");
        		strcat(message, arg);
        		strcat(message, " second timer has finished!\n\n");
        		alarm(seconds);
        	}
        }

        /*kill*/
        else if(strncmp(cmd, "kill", 4) == 0){
        	char *arg = strtok(cmd, " ");
        	char *pid = strtok(NULL, " ");
        	char *temp = strtok(NULL, " ");
        	int ID = atoi(pid);

        	/*If 'kill' is followed by other characters*/
        	if(strcmp(arg, "kill") != 0){
        		fprintf(stderr, "%s: Unkown command\n", arg);
        		continue;
        	}

        	/*If kill does not have an argument*/
        	if(pid == NULL){
        		fprintf(stderr, "ERROR: Not enough arguments for kill\n");
        		continue;
        	}

        	/*If kill has too many arguments*/
        	if(temp != NULL){
        		fprintf(stderr, "ERROR: Too many arguments for kill\n");
        		continue;
        	}

        	/*Attempt to kill process*/
        	if(kill(ID, SIGKILL) == -1){

        		/*If the process could not be killed*/
        		fprintf(stderr, "ERROR: Could not kill process\n");
        		continue;
        	}
        }

        /*Executables*/
        else{
        	char *first = strtok(cmd, " ");
        	char **arguments = (char**)malloc(70 * sizeof(char*));
        	arguments[0] = first;


        	/*User pressed enter for no reason*/
        	if(first == NULL){
        		continue;
        	}

        	/*create the arguments and array for exec*/
        	int i = 1;
        	char *leftover;
        	char *temp;
        	while((leftover = strtok(NULL, "")) != NULL){
        		if(leftover[0] == '"'){
        			temp = strtok(leftover+1, "\"");
        			arguments[i] = temp;
        			i++;
        		}
        		else{
        			temp = strtok(leftover, " ");
        			arguments[i] = temp;
        			i++;
        		}
        	}
        	arguments[i] = NULL;
        	i = 0;
        	while(arguments[i] != NULL){
        		i++;
        	}

        	/*Detect if a redirection is needed*/
        	int redirectFinder = 0;
        	char **execute = (char**)malloc(70 * sizeof(char*));
        	char **execute2 = (char**)malloc(70 * sizeof(char*));
        	int toOut = 0;
        	int toIn = 0;
        	int toErr = 0;
        	int toApp = 0;
        	int toInAndOut = 0;
        	int toPipe = 0;
        	int toBoth = 0;
        	char *toOutfd = NULL;
        	char *toInfd = NULL;
        	char *toErrfd = NULL;
        	char *toAppfd = NULL;
        	char *tobothfd = NULL;
        	while(arguments[redirectFinder] != NULL){

        		/*Is redirection a '<', '>' or '|'*/
        		if(strcmp(arguments[redirectFinder], ">") == 0 || strcmp(arguments[redirectFinder], "1>") == 0 || strcmp(arguments[redirectFinder], "2>") == 0 || strcmp(arguments[redirectFinder], ">>") == 0 || strcmp(arguments[redirectFinder], "&>") == 0){
        			if(strcmp(arguments[redirectFinder], "2>") == 0){
        				toErr = 1;
        				toErrfd = arguments[redirectFinder+1];
        			}
        			else if(strcmp(arguments[redirectFinder], ">>") == 0){
        				toApp = 1;
        				toAppfd = arguments[redirectFinder+1];
        			}
        			else if(strcmp(arguments[redirectFinder], "&>") == 0){
        				toBoth = 1;
        				tobothfd = arguments[redirectFinder+1];
        			}
        			else{
        				toOut = 1;
        				toOutfd = arguments[redirectFinder+1];
        			}
        			arguments[redirectFinder] = NULL;
        			break;
        		}

        		else if(strcmp(arguments[redirectFinder], "<") == 0){
        			toIn = 1;
        			toInfd = arguments[redirectFinder+1];
        			if(arguments[redirectFinder+2] != NULL){
        				if(strcmp(arguments[redirectFinder+2], ">") == 0){
        					toInAndOut = 1;
        					toOutfd = arguments[redirectFinder+3];
        				}
        			}
        			arguments[redirectFinder] = NULL;
        			break;
        		}

        		else if(strcmp(arguments[redirectFinder], "|") == 0){
        			toPipe++;
        			arguments[redirectFinder] = NULL;
        			redirectFinder++;
        			int executeIndex = 0;
        			int executeIndex2 = 0;
        			while(arguments[redirectFinder] != NULL && strcmp(arguments[redirectFinder], "|") != 0){
        				execute[executeIndex] = arguments[redirectFinder];
        				executeIndex++;
        				redirectFinder++;
        				if(arguments[redirectFinder] != NULL && strcmp(arguments[redirectFinder], "|") == 0){
        					toPipe++;
        					execute[executeIndex] = NULL;
        					redirectFinder++;
        					while(arguments[redirectFinder] != NULL){
        						execute2[executeIndex2] = arguments[redirectFinder];
        						redirectFinder++;
        						executeIndex2++;
        					}
        					break;
        				}
        			}
        			break;
        		}
        		else
        			redirectFinder++;
        	}

        	struct stat checkExists;
        	/*'/' found, no need to search the path*/
        	if(strchr(first, '/') != NULL){

        		/*If the file/program exists*/
        		if(stat(first, &checkExists) == 0){
        			int pid;

        			/*child*/
        			if((pid = fork()) == 0){
        				/*Redirect output*/
        				if(toOut == 1){
        					FILE *fd;
        					fd = fopen(toOutfd, "w");
        					dup2(fileno(fd), 1);
        				}
        				else if(toErr == 1){
        					FILE *fd;
        					fd = fopen(toErrfd, "w");
        					dup2(fileno(fd), 2);
        				}
        				else if(toApp == 1){
        					FILE *fd;
        					fd = fopen(toAppfd, "a");
        					dup2(fileno(fd), 1);
        				}
        				else if(toBoth == 1){
        					FILE *fd;
        					fd = fopen(tobothfd, "w");
        					dup2(fileno(fd), 1);
        					dup2(fileno(fd), 2);
        				}
        				/*Redirect input*/
        				else if(toIn == 1){
        					FILE *fd;
        					fd = fopen(toInfd, "r");
        					dup2(fileno(fd), 0);
        					if(toInAndOut == 1){
        						FILE *fd2;
        						fd2 = fopen(toOutfd, "w");
        						dup2(fileno(fd2), 1);
        					}
        				}

        				/*Piping*/
        				if(toPipe == 1){
        					int pipefd[2];
        					int child, child_status;
        					if(pipe(pipefd) == -1){
        						fprintf(stderr, "ERROR: Could not creat pipe\n");
        						continue;
        					}

        					/*(Creating child of the child)*/
        					if((child = fork()) == 0){

        						/*Set second program to read from First program*/
        						close(pipefd[0]);
        						dup2(pipefd[1], 1);
        						if(execv(arguments[0], arguments)){
        							fprintf(stderr, "ERROR: Could not execute program\n");
        							continue;
        						}
        						exit(0);
        					}
        					else{
        						wait(&child_status);
        						close(pipefd[1]);
        						dup2(pipefd[0], 0);
        						if(execv(execute[0], execute)){
        							fprintf(stderr, "ERROR: Could not execute program\n");
        							continue;
        						}
        					}
        				}
        				if(execv(arguments[0], arguments)){
        					fprintf(stderr, "ERROR: Could not execute program\n");
        					continue;
        				}
        			}

        			/*parent wait for child*/
        			else{

        				int child_status;
        				wait(&child_status);
        			}
        		}

        		/*If the file/program does not exist*/
        		else{
        			fprintf(stderr, "ERROR: File does not exist\n");
        			continue;
        		}
        	}

        	/*If program path not given, search PATH environement*/
        	else{
				int numChildren = 0;

        		int pid, child_status;

        		numChildren++;
        		numChildren = numChildren + toPipe;
        		/*Create child process*/
        		pid = fork();
        		if(pid == 0){
        			int i = 0;
        			int failed = 1;
					/*concatenate the relative path to the entries of paths*/
					while(failed != 0){
						failed--;
        				while(paths[i] != NULL){
        					char *tempo = malloc(200);
        					strcpy(tempo, paths[i]);
        					strcat(tempo, "/");
        					strcat(tempo,arguments[0]);

        					if(toOut == 1){
        						FILE *fd;
        						fd = fopen(toOutfd, "w");
        						dup2(fileno(fd), 1);
        					}
        					else if(toErr == 1){
        						FILE *fd;
        						fd = fopen(toErrfd, "w");
        						dup2(fileno(fd), 2);
        					}
        					else if(toApp == 1){
        						FILE *fd;
        						fd = fopen(toAppfd, "a");
        						dup2(fileno(fd), 1);
        					}
        					else if(toBoth == 1){
        						FILE *fd;
        						fd = fopen(tobothfd, "w");
        						dup2(fileno(fd), 1);
        						dup2(fileno(fd), 2);
        					}
        					else if(toIn == 1){
        						FILE *fd;
        						fd = fopen(toInfd, "r");
        						dup2(fileno(fd), 0);
        						if(toInAndOut == 1){
        							FILE *fd2;
        							fd2 = fopen(toOutfd, "w");
        							dup2(fileno(fd2), 1);
        						}
        					}
        					else if(toPipe != 0 && stat(tempo, &checkExists) == 0){
        						int pipefd[2];
        						singlePipe(pipefd, tempo, arguments, execute, execute2, paths, toPipe);
        					}
        					if(execv(tempo, arguments)){
        						failed = 1;
        					}
        					i++;
        					free(tempo);
        				}
        			}
        			fprintf(stderr, "%s: command not found\n", arguments[0]);
        		}
        		else{
        			while(numChildren != 0){
        				waitpid(-1, &child_status, 0);
        				numChildren--;
        			}
        		}
        	}
        	free(arguments);
        	free(execute);
        	free(execute2);
        }

        /* All your debug print statements should use the macros found in debu.h */
        /* Use the `make debug` target in the makefile to run with these enabled. */
        //info("Length of command entered: %ld\n", strlen(cmd));
        /* You WILL lose points if your shell prints out garbage values. */

        printf("\n");

        /*Keep track of prompt*/
        prompt[13] = '\0';
        getcwd(cwd, sizeof(cwd));
        strcat(cwd, " $");
        strcat(prompt, cwd);
    }


    /* Don't forget to free allocated memory, and close file descriptors. */
    free(cmd);
    free(paths);

    if((sigprocmask(SIG_SETMASK, &prev_mask, NULL)) == -1){
		fprintf(stderr, "ERROR: sigprocmask could not execute, SIGTSTP still blocked\n");
	}

    return EXIT_SUCCESS;
}
