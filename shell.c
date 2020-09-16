#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>


int ARG_SIZE = 4096;
int NUM_ARGS = 128;
int status;
int state = 1;
int in = 0;
int out = 0;

void get_token (char **array, char *command) {
    *array++ = strtok(command, " \t\n");
    while (*(array - 1) != NULL && strcmp(*(array - 1), "\n") != 0) {
        *array++ = strtok(NULL, " \t\n");
    }
    *array = '\0';
}       

int run_command (char **array) {
    int exec_stat;
    int cpid;
    cpid = fork();
	if (cpid > 0) {
        if (state == 1) {// wait for command
	        waitpid(cpid,&status, 0);
            if (WEXITSTATUS(status) == 2) {
                printf("Invalid command\n");
            }
            return 0;
        } else {
            return cpid;
        } 
    } else { 
        if (in != 0) {
            close(0);
            dup(in);
        } 
        if (out != 0) {
            close(1);
            dup(out);
        }
	    exec_stat = execvp(array[0], array);
        close(0);
        close(1);
        if (exec_stat == -1) {
            exit(errno);
        }

    }
}

int main () {
    int PROMPT_SIZE = 64;
    struct utsname system;
    uname(&system);
    char *user = getlogin();
    char *prompt;
    int bg = 0;
    int back_pid = -1;
    int pids[NUM_ARGS];
    prompt = malloc(sizeof(char) * PROMPT_SIZE);
    snprintf(prompt, PROMPT_SIZE, "\033[1;32m%s@%s\033[1;34m~>\n> \033[0m", user, system.nodename);
    // I like colorful multiline prompts

    char *command = malloc(sizeof(char) * ARG_SIZE * NUM_ARGS);    
    char *tokens[NUM_ARGS];
    char *in_status;
    char a,c;
    int nsep, nin, nout, npipe;
    int dir;
    int take_pipe = 0;
    char *current;
    int pipefd[2];
    FILE *temp;
    while (1) {
        state = 1;

        printf("%s",prompt);
        in_status = fgets(command, ARG_SIZE * NUM_ARGS, stdin);
        if (in_status == NULL) // Check if EOF was found
            exit(0);
        strtok(command, "\n");
        current = command;
        // Look for command sequencing with ; or &
        while (current[0] != '\0' && current[0] != '\n') {
            in = 0;
            out = 0;
            nsep = strcspn(current, "&;|"); // Get pos of the first ; or &
            nin = strcspn(current, "<"); // Get pos of the first <
            nout = strcspn(current, ">"); // Get pos of the first >
            //npipe = strcspn(current, "|"); // Get pos of the first |

            // Process all redirections
            if (nin < nsep) { // this command must be redirected
                char *file = strtok(current + nin + 1, " \t\n;&><");
                if ((temp = fopen(file, "r")) == NULL)
                    printf("FOPEN IN ERROR");
                else
                    in = fileno(temp);
                current[nin] = '\0';
            }
            if (nout < nsep) {
                char *file = strtok(current + nout + 1, " \t\n;&><");
                if (current[nout + 1] == '>') {
                    temp = fopen(file, "a");
                } else {
                    temp = fopen(file, "w");
                }
                if (temp == NULL)
                     printf("FOPEN OUT ERROR");
                else
                    out = fileno(temp);
                current[nout] = '\0';
            }

            if (take_pipe) {
                in = pipefd[0];
                take_pipe = 0;
            }

            // Process seperators
            if (current[nsep] == ';') {
                state = 1;
                current[nsep] = '\0';
                get_token(tokens, current);
            } else if (current[nsep] == '&') {
                state = 0;
                current[nsep] = '\0';
                get_token(tokens, current);
            } else if (current[nsep] == '|') {
               pipe(pipefd);
               out = pipefd[1];
               take_pipe = 1;
               state = 1;
               current[nsep] = '\0';
               get_token(tokens, current);
            } else { 
                state = 1;
                get_token(tokens, current);
            }

            // Check for built ins like exit or cd
            if (strcmp(tokens[0], "exit") == 0) {
                exit(0);
            } else if (strcmp(tokens[0], "cd") == 0) {
                dir = chdir(tokens[1]);
                if (dir != 0) {
                    if (errno == 2) {
                        printf("cd: directory %s does not exist\n", tokens[1]);    
                    }
                }
            } else {
                back_pid = run_command(tokens);
                if (take_pipe) {
                    close(pipefd[1]);
                }
            
                if (back_pid != 0) {
                    bg++; // Add the pid as a background process to keep track of
                    pids[bg] = back_pid;
                }
            }
            current += (nsep + 1); // Move current to point to the first char after the sequencing character, or if none are found, it will move to the null pointer breaking the while loop
        }
        if (bg > 0) { // There are background processes
            for (int i = 1; i <= bg; i++) {
                if (waitpid(pids[i], &status, WNOHANG) != 0) { //Check if they are dont without blocking
                    printf("\n[%d] DONE %d\n", i, pids[i]);
                    bg--;
                } else {
                    printf("[%d] %d\n", i, pids[i]); 
                }
            }
        }
    }   
}
