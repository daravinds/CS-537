#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

int isBuiltInCommand(char* token);
void executeBuiltInCommand(char** argv, pid_t backgroundPids[], int bgPidCount);

int main() {
  pid_t pid;
  pid_t backgroundPids[20];
  memset(backgroundPids, -1, 20 * sizeof(pid_t));
  char* command;
  command = malloc(128);
  unsigned int stdin = STDIN_FILENO;
  unsigned int stdout = STDOUT_FILENO;
  unsigned int commandCount = 1;
  int childStatus, isBackground = 0, bgPidCount = 0;

  while(1) {
    isBackground = 0;
    fflush(fdopen(stdout, "w"));
    printf("mysh (%u)> ", commandCount);
    char* success = fgets(command, 128, fdopen(stdin, "r"));
    if(success == NULL)
      exit(EXIT_FAILURE);
    success[strlen(success) - 1] = '\0';

    char** argv = malloc(128 * sizeof(char*));
    char *token;
    int tokenCount = 0;
    while((token = strtok_r(command, " \t", &command))) {
      argv[tokenCount] = strdup(token);
      tokenCount++;
    }
    argv[tokenCount] = NULL;
    if(strcmp(argv[tokenCount - 1], "&") == 0) {
      isBackground = 1;
      signal(SIGCHLD, SIG_IGN);
      argv[tokenCount - 1] = NULL;
    }
    if(isBuiltInCommand(argv[0])) {
      executeBuiltInCommand(argv, backgroundPids, bgPidCount);
    } else {
      pid = fork();
      if(pid < 0) {
        exit(EXIT_FAILURE);
      }
      else if (pid == 0) {
        execvp(argv[0], argv);
        printf("Execution failed with err no: %d\n", errno);
      }
      else {
        if(isBackground) {
          backgroundPids[bgPidCount] = pid;
          bgPidCount++;
        }
        else {
          (void) waitpid(pid, &childStatus, 0);
        }
      }
    }
    commandCount++;
  }
  exit(EXIT_SUCCESS);
}

int isBuiltInCommand(char* token) {
  if(strcmp(token, "exit") == 0 || strcmp(token, "pwd") == 0 || strcmp(token, "cd") == 0) {
    return 1;
  }
  return 0;
}

void executeBuiltInCommand(char** argv, pid_t backgroundPids[], int bgPidCount) {
  int i;
  if(strcmp(argv[0], "pwd") == 0) {
    char *cwd = malloc(128);
    printf("%s\n", getcwd(cwd, 128));
  } else if (strcmp(argv[0], "exit") == 0) {
    for(i = 0; i < bgPidCount; i++)
    {
      if(kill(backgroundPids[i], 0) == 0) {
        // if(backgroundPids[i] != -1)
        kill(backgroundPids[i], SIGKILL);
        // printf("Background job ID: %d\n", backgroundPids[i]);
      }
    }
    exit(EXIT_SUCCESS);
  } else if (strcmp(argv[0], "cd") == 0) {
    if(argv[1] == NULL) {
      chdir(getenv("HOME"));
    } else {
      chdir(argv[1]);
    }
  }
}
