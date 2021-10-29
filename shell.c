/**
 * @author  Marissa Christenson, 2000064
 * @since   04/09/21
 * shell.c
 *
 * This is a representation of the Unix shell. It takes commands from the user
 * until the user inputs the exit command. It runs several system calls, including 
 * but not limited to ls, sleep, wc, etc. The calls will also take parameters which 
 * start with a hyphen. There can be individual parameters or several parameters
 * with one hyephen. 
 * 
 * The program will allow for single piping action and redirections with |, <, and 
 * >. Also included is a history command that will call the previously entered 
 * command. The program also allows the child program to run cocurrently with '&'.
 *
**/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> // fork, wait
#include <sys/wait.h>  // wait
#include <unistd.h>    // fork
#include <stdlib.h>    // for exit
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <regex.h>

#define MAX_LINE 80 /* The maximum length command */

/**
 * This is my main function in which most of the shell operations occur. It 
 * takes no parameters. Thre are several variables within the method that are 
 * used by the parent and children processes so the shell can function properly.
 * @return integer
 * @custom.preconditions  No preconditions
 * @custom.postconditions  No postconditions
 **/
int main(void)
{
   //enums represting pipe and redirection or the lack of
   enum action
   {
      PIPE = '|',
      REDIRECTOUT = '>',
      REDIRECTIN = '<',
      NONE
   };

   //this variable represents whether a pipe or redirection action is necessary
   enum action actionTaken = NONE;
   
   char* piped = "|";
   char* redirectIn = "<";
   char* redirectOut = ">";

   //delimiters used to separate tokens
   char delimiters[] = " \t\r\n\v\f";
   
   //reg expression to search for action
   char* actions = "[&|><]";
   regex_t re;
   regcomp(&re, actions, 0);

   //holds string value for history command
   char *historyCommand = "!!";
   //holds string value for concurrent execution command
   char *waitingCommand = "&";
   //holds string value for exit command
   char *exitCommand = "exit";

   //location of second arg within array
   int actionPosition = 1;

   //if input has ever been processed
   bool historyExists = false;
   
   //concurrent execution occurs if true
   bool waiting = true;

   //Process command input
   char *argsbuf[MAX_LINE / 2 + 1] = {NULL};
   char** c = argsbuf;

   //Run command input
   char *argsbuf2[MAX_LINE / 2 + 1] = {NULL};
   char** d = argsbuf2;

   //flag to exit program
   int should_run = 1;
   
   while (should_run)
   {
      //initial char array of input
      char buf[MAX_LINE + 1];

      if(waiting) {
         wait(NULL);
      }

      printf("osh> ");
      fflush(stdout);

      /**
       * The input is taken from the command line by fgets()
       * and stored in the string buf
       **/
      if (fgets(buf, MAX_LINE, stdin))
      {
         //process input line
         char *token = strtok(buf, delimiters);
         while (token)
         {
            *c = malloc(strlen(token) + 1 * sizeof(char));
            strcpy(*c, token);
            c++;
            token = strtok(NULL, delimiters);
         }
         c = argsbuf;

         if(*c)
         {
            if(strcmp(*c, exitCommand) != 0)
            {
               //check if using history 
               bool notUsingHist = (strcmp(*c, historyCommand) != 0);
               bool useHistAndExists = (!notUsingHist && historyExists);

               /**
                * Decide to use the previous command a new command. 
                * No previous command, if statement  not entered/ prompt is shown to user
                **/
               if (notUsingHist || useHistAndExists)
               {
                  if (notUsingHist)
                  {
                     //printf("Entered notUsingHist\n");
                     actionTaken = NONE;
                     actionPosition = 1;
                     waiting = true;
                     historyExists = true;

                     //copying new command into array holding previous command
                     //checking for action/ actionPosition
                     //checking for ampersand (is shell waits)
                     int commPos = 0;
                     char** tempC = argsbuf;
                     while(*c) {
                        if(*d) {
                           free(*d);
                        }
                        *d = *c;
                        *c = NULL;
                        
                        if(actionTaken == PIPE) {
                           *tempC = *d;
                           tempC++;
                        }

                        int regValue = regexec(&re, *d, 0, NULL, 0);
                        if(regValue == 0) {
                           //pipe
                           if(strcmp(*d, piped) == 0) {
                              actionTaken = PIPE;
                              actionPosition += commPos;

                           //redirectIn
                           }else if(strcmp(*d, redirectIn) == 0) {
                              actionTaken = REDIRECTIN;
                              actionPosition += commPos;
                           
                           //redirectOut
                           }else if(strcmp(*d, redirectOut) == 0) {
                              actionTaken = REDIRECTOUT;
                              actionPosition += commPos;
                           
                           //waitingCommand   
                           }else if(strcmp(*d, waitingCommand) == 0) {
                              waiting = false;
                           }

                           free(*d);
                           *d = NULL;
                        }

                        c++;
                        d++;
                        commPos++;
                     }

                     if(*d) {
                        free(*d);
                        *d = NULL;
                     }

                     c = argsbuf;
                     d = argsbuf2;
                  }else {
                     //if using history and it exists
                     int i = actionPosition;
                     while(*c) {
                        free(*c);
                        *c = NULL;
                        if(actionTaken == PIPE) {
                           *c = argsbuf2[i++];
                        }
                        c++;
                     }
                     c = argsbuf;
                  }

                  //FIRST FORK
                  int pid = fork();

                  if (pid < 0) {
                     perror("Error during fork\n");
                     exit(1);
                  }

                  if ((pid > 0) && (waiting)) {
                     wait(NULL);
                     while(*c) {
                        *c = NULL;
                        c++;
                     }
                     c = argsbuf;
                  }

                  if (pid == 0) {
                     //FIRST CHILD
                     if (actionTaken == PIPE)
                     {
                        int pipeFD[2];

                        if (pipe(pipeFD) < 0)
                        {
                           perror("Error in creating pipe\n");
                           exit(1);
                        }

                        //SECOND FORK
                        int pid2 = fork();

                        if (pid2 < 0)
                        {
                           perror("Error during fork\n");
                           exit(1);
                        }

                        if (pid2 > 0)
                        {
                           wait(NULL);
                           close(pipeFD[1]);
                           dup2(pipeFD[0], 0);
                           //execvp(argsbuf2[0], argsbuf2);
                           //execvp(argsbuf2[actionPosition], argsbuf2);
                           execvp(argsbuf[0], argsbuf);
                           exit(0);
                        }
                        else if (pid2 == 0)
                        {
                           // SECOND CHILD
                           close(pipeFD[0]);
                           dup2(pipeFD[1], 1);
                           //execvp(argsbuf[0], argsbuf);
                           execvp(argsbuf2[0], argsbuf2);
                           exit(0);
                        }
                     }
                     else if (actionTaken == REDIRECTOUT)
                     {
                        //‘>’ redirects the output of a command to a file
                        //int fileFD = open(argsbuf2[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                        int fileFD = open(argsbuf2[actionPosition], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                        if (fileFD < 0)
                        {
                           perror("Error opening file\n");
                           exit(1);
                        }
                        dup2(fileFD, 1);
                        close(fileFD);
                        //execvp(argsbuf[0], argsbuf);
                        execvp(argsbuf2[0], argsbuf2);
                        exit(0);
                     }
                     else if (actionTaken == REDIRECTIN)
                     {
                        //‘<’ redirects the input to a command from a file
                        //int fileFD = open(argsbuf2[0], O_RDONLY, 0666);
                        int fileFD = open(argsbuf2[actionPosition], O_RDONLY, 0666);
                        if (fileFD < 0)
                        {
                           perror("Error opening file\n");
                           exit(1);
                        }
                        dup2(fileFD, 0);
                        //execvp(argsbuf[0], argsbuf);
                        execvp(argsbuf2[0], argsbuf2);
                        exit(0);
                     }
                     else
                     {
                        //execvp(argsbuf[0], argsbuf);
                        execvp(argsbuf2[0], argsbuf2);
                        exit(0);
                     }
                  }
               } else
               {
                  //deallocate memory
                  while(*c) {
                     free(*c);
                     *c = NULL;
                     c++;
                  }
                  c = argsbuf;
                  printf("No commands in history\n");
                  fflush(stdout);
               }
            }else {
               //printf("ENTERED EXIT\n");
               should_run = 0;
               regfree(&re);
               c = argsbuf;
               d = argsbuf2;
               for(int i = 0; i < MAX_LINE / 2 + 1; i++) {
                  if(*c) {
                     free(*c);
                     *c = NULL;
                  }
                  if(*d) {
                     free(*d);
                     *d = NULL;
                  }
                  c++;
                  d++;
               }
            }
         }
      }
   }
   return 0;
}