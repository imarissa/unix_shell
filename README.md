### UNIX Shell
***
The UNIX shell consists of a shell interface that accepts UNIX commands and executes each command in a separate process. It supports input and output redirection, as well as pipes as a 
form of IPC between a pair of commands. Once the program is run, the prompt osh> will appear and the user can enter UNIX commands that will be performed on their system. Example commands 
include 'ls', 'ls | wc', 'sleep 5 &', 'ls -al', a history command '!!', etc. A UNIX operating system is required, as the UNIX shell uses underlying UNIX program files to execute the command. 

No command line arguments are required.

#### Files
***
The shell.c is included. It provides the following general operations of a command-line shell:
* Presents prompt osh->
* Reads user input 
* Handles incorrect input and executes UNIX commands 
* Handles interprocess communication including: history feature, !!, waiting mechanism, &, and a single instance of IPC including piping, |, redirect in, <, and redirect out, >.
* Continually loops until the user enters 'exit' at prompt

Languages used: C

#### Compilation & Running
***
Generate executable:
```sh
gcc shell.c –o shell –pthread
```
Run from command line:
```sh
./shell
```
