#include <stdlib.h> 
#include <stdio.h> 
#include <unistd.h>
#include <signal.h> 

void handler(int sig) { 
    printf("You think hitting ctrl-c will stop the bomb?\n"); 
    sleep(2); 
    printf("Well..."); 
    fflush(stdout); 
    sleep(1); 
    printf("OK\n"); 
    exit(0); 
} 
 
void main() { 
    signal(SIGINT, handler); /* installs ctl-c handler */
    while(1) { 
    } 
} 
