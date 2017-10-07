#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define MAX_BG_JOBS 20
#define MAX_COMMAND_LENGTH 128

int isBuiltInCommand(char* token);
void executeBuiltInCommand(char** argv, pid_t backgroundPids[], int bgPidCount);
void executeCommand(char** argv, int isBackground, int backgroundPids[], int* bgPidCount, char* inputFile, char* outputFile);
void printErrorMessage();
int validInputAndOutputFiles(char** argv, int ipIndex, int opIndex, char** ipFile, char** opFile);
void setInputFd(char* inputFile);
void setOutputFd(char* outputFile);
void handlePipeOperation(char** argv);
void p(int a);

int main(int arg_count, char *argv_main[]) {
  if(arg_count != 1) {
    printErrorMessage();p(1);
    exit(1);
  }

  pid_t backgroundPids[MAX_BG_JOBS];
  memset(backgroundPids, -1, MAX_BG_JOBS * sizeof(pid_t));
  char command[2 * MAX_COMMAND_LENGTH];
  char** argv;
//  command = malloc(2 * MAX_COMMAND_LENGTH * sizeof(char));
  if(command == NULL) {
    printErrorMessage();p(2);
    exit(1);
  }
  
  unsigned int commandCount = 1;
  int isBackground = 0, bgPidCount = 0, isPipeOperation = 0;
  argv = malloc(MAX_COMMAND_LENGTH * sizeof(char*));
  if(argv == NULL) {
    printErrorMessage();p(3);
    exit(1);
  }

  memset(argv, 0, MAX_COMMAND_LENGTH * sizeof(char *));
  while(1) {
    isBackground = 0;
    isPipeOperation = 0;
    printf("mysh (%u)> ", commandCount);
    fflush(stdout);
    fgets(command, 5 * MAX_COMMAND_LENGTH, stdin);
    command[strlen(command) - 1] = '\0';
    if(strlen(command) > MAX_COMMAND_LENGTH) {
      commandCount++;
      printErrorMessage();p(3);
      continue;
    }
    char *token;
    int inputRedirectionIndex = -1, outputRedirectionIndex = -1;
    int tokenCount = 0;

    char* saveptr, *commandPtr;
    commandPtr = command;
    while((token = strtok_r(commandPtr, " ", &saveptr))) {
      if((isPipeOperation == 0) && (strcmp(token, "|") == 0))
        isPipeOperation = 1;
//      if(tokenCount > 0) {
        if(strcmp(token, "<") == 0)
          inputRedirectionIndex = tokenCount;
        else if(strcmp(token, ">") == 0)
          outputRedirectionIndex = tokenCount;
  //    }
      argv[tokenCount] = token;
      tokenCount++;
      commandPtr = NULL;
    }

    argv[tokenCount] = NULL;
    if(tokenCount == 0)
      continue;

    if(isPipeOperation == 1) {
      handlePipeOperation(argv);
      commandCount++;
      continue;
    }

    if(strcmp(argv[tokenCount - 1], "&") == 0) {
      isBackground = 1;
      signal(SIGCHLD, SIG_IGN);
      argv[tokenCount - 1] = NULL;
    }

    char* inputFile, *outputFile;
    inputFile = outputFile = NULL;
    int valid = validInputAndOutputFiles(argv, inputRedirectionIndex, outputRedirectionIndex, &inputFile, &outputFile);
    if(valid == 0) {
      printErrorMessage();p(4);
      commandCount++;
      continue;
    }

    if(inputRedirectionIndex != -1)
      argv[inputRedirectionIndex] = NULL;
    if(outputRedirectionIndex != -1)
      argv[outputRedirectionIndex] = NULL;
    
    if(isBuiltInCommand(argv[0])) {
      executeBuiltInCommand(argv, backgroundPids, bgPidCount);
    } else {

      executeCommand(argv, isBackground, backgroundPids, &bgPidCount, inputFile, outputFile);
    }
    commandCount++;
  }
  exit(0);
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
    if(argv[1] != NULL) {
      printErrorMessage();
      return;
    }
    char* pwd = getcwd(NULL, 0);
    printf("%s\n", pwd);
    free(pwd);
  } else if (strcmp(argv[0], "exit") == 0) {
    for(i = 0; i < bgPidCount; i++) {
      if(kill(backgroundPids[i], 0) == 0)
        kill(backgroundPids[i], SIGKILL);
    }
    free(argv);
    exit(0);
  } else if (strcmp(argv[0], "cd") == 0) {
    if(argv[1] == NULL) {
      chdir(getenv("HOME"));
    } else {
      if(chdir(argv[1]) == -1)
        printErrorMessage();
    }
  }
}

void executeCommand(char** argv, int isBackground, int backgroundPids[], int* bgPidCount, char* inputFile, char* outputFile) {
  pid_t pid = fork();
  if(pid < 0) {
    printErrorMessage();p(5);
    exit(1);
  }
  else if (pid == 0) {
    setInputFd(inputFile);
    setOutputFd(outputFile);
    execvp(argv[0], argv);
    printErrorMessage();
    exit(1);
  }
  else
    if(isBackground){
      if(*bgPidCount < MAX_BG_JOBS)
        backgroundPids[(*bgPidCount)++] = pid;
      else {
        int i;
        for(i = 0; i < *bgPidCount; i++)
          if(kill(backgroundPids[i], 0) != 0) {
            backgroundPids[i] = pid;
            break;
          }
      }
    }
    else {
      int childStatus;
      (void) waitpid(pid, &childStatus, 0);
    }
}

void printErrorMessage() {
  char errorMessage[30] = "An error has occurred\n";
  write(STDERR_FILENO, errorMessage, strlen(errorMessage));
}

int validInputAndOutputFiles(char** argv, int ipIndex, int opIndex, char** ipFile, char** opFile) {
  int i = ipIndex + 1;
  if(ipIndex != -1) {
    while((argv[i] != NULL) && (strcmp(argv[i], ">") != 0)) {
      *ipFile = argv[i];
      i++;
    }
    if(*ipFile == NULL)
      return 0;
    else if(strcmp(*ipFile, argv[ipIndex + 1]) != 0)
      return 0;
  }
  
  i = opIndex + 1;
  if(opIndex != -1) {
    while((argv[i] != NULL) && (strcmp(argv[i], "<") != 0)) {
      *opFile = argv[i];
      i++;
    }
    if(*opFile == NULL)
      return 0;
    else if(strcmp(*opFile, argv[opIndex + 1]) != 0)
      return 0;
  }
  return 1;
}

void setInputFd(char* inputFile) {
  if(inputFile != NULL) {
    int inputFd = open(inputFile, O_RDONLY, S_IRUSR);
    if(inputFd == -1) {
      printErrorMessage();
      exit(1);
    }
    dup2(inputFd, STDIN_FILENO);
  }
}

void setOutputFd(char* outputFile) {
  if(outputFile != NULL) {
    int outputFd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(outputFd == -1) {
      printErrorMessage();
      exit(1);
    }
    dup2(outputFd, STDOUT_FILENO);
  }
}

void handlePipeOperation(char** argv) {
  int index = 0, i = 0;
  char** command_1_array, **command_2_array;
  command_1_array = malloc(MAX_COMMAND_LENGTH * sizeof(char*));
  if(command_1_array == NULL) {
    printErrorMessage();p(6);
    exit(1);
  }
  command_2_array = malloc(MAX_COMMAND_LENGTH * sizeof(char*));
  if(command_2_array == NULL) {
    printErrorMessage();p(7);
    exit(1);
  }

  while(strcmp(argv[index], "|") != 0) {
    command_1_array[i] = argv[index++];
    i++;
  }
  if(i == 0) {
    printErrorMessage();
    return;
  }
  command_1_array[i] = NULL;

  i = 0;
  index++;
  while(argv[index] != NULL) {
    command_2_array[i] = argv[index++];
    i++;
  }
  if(i == 0) {
    printErrorMessage();
    return;
  }
  command_2_array[i] = NULL;
  pid_t pid_1, pid_2;

  pid_1 = fork();
  if(pid_1 < 0) {
    printErrorMessage();
    exit(1);
  } else if(pid_1 == 0) {
    int fd[2];
    int pipeSuccess = pipe(fd);
    if(pipeSuccess < 0) {
      printErrorMessage();
      exit(1);
    }
    pid_2 = fork();
    if(pid_2 < 0) {
      printErrorMessage();
      exit(1);
    } else if(pid_2 == 0) {
      close(STDOUT_FILENO);
      close(fd[0]);
      dup2(fd[1], STDOUT_FILENO);
      execvp(command_1_array[0], command_1_array);
      printErrorMessage();
      exit(1);
    } else {
      close(STDIN_FILENO);
      close(fd[1]);
      dup2(fd[0], STDIN_FILENO);
      execvp(command_2_array[0], command_2_array);
      printErrorMessage();
      exit(1);
    }
  } else {
    waitpid(pid_1, NULL, 0);
    free(command_1_array);
    free(command_2_array);
  }
}

void p(int a) {
//  printf("%d\n", a);
}
