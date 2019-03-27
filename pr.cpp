
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>


//file location stored on a shareable directory.
const char * chatFileLocation = "/tmp/chat.txt";

//store other process's pid.
int otherPrId = 0;

//a flag variable to check if we received
//other process's pid from the chat file on startup.
bool otherPrIdflag = false;

//an integer variable to store the process type.
//on startup set this to 1 or 2.
int type = 0;

bool readFileByLine(const char *fileLocation);
bool readFileExtractPid(const char *fileLocation);
void writeFile(const char *fileLocation, char text[]);
void writeFile(const char *fileLocation, long pid);
static void sigint(int sig);
static void sigusr(int sig);


void onExit(){
    remove(chatFileLocation);
    printf("Terminated.\n %s is deleted.", chatFileLocation);
    fflush(stdout);
}


int main(int argc, char *argv[])
{
    if(argc < 2){
        printf("Usage: %s [1 or 2] \n", argv[0]);
        return 0;
    }

    if(atoi(argv[1]) == 1){
        type = 1;
        if (signal(SIGUSR1, sigusr) == SIG_ERR){
        printf("Unable to create handler for SIGUSR1\n");
        return 0;
        }
    }else if(atoi(argv[1]) == 2){
        type = 2;
        if (signal(SIGUSR2, sigusr) == SIG_ERR){
        printf("Unable to create handler for SIGUSR2\n");
        return 0;
        }
    }

   
    //delete chat file if interrupt signal is received
    if (signal(SIGINT, sigint) == SIG_ERR){
        printf("Unable to create handler for SIGINT\n");
        printf("Please manually delete %s upon termination.", chatFileLocation);
    }

    //ensuring chat file deletion upon exit
    atexit(onExit);

    //self signaling to initiate pid sharing
    sigusr(10);
    
    //infinite while loop to keep the thread active
    while(1){
        //pause will end when this process receives a signal
        pause();
    }

    return 0;
}

bool readFileByLine(const char *fileLocation){
    FILE *fpr;
    //printf("readFileByLine");
    fpr = fopen(fileLocation, "r");
    if (fpr == NULL){
        return false;
    }

    char str[1024];
    fgets(str, 1024, fpr);
    printf("Message received: %s \n", str);
    fflush(stdout);

    //reallocate memory space - not necessarily needed but
    //it is a good practice 
    memset(str, 0, sizeof str);

    return true;
}

bool readFileExtractPid(const char *fileLocation){
    FILE *fpr;
    //printf("readFileExtractPid ");
    fpr = fopen(chatFileLocation, "r");
    if (fpr == NULL){
        return false;
    }

    char str[1024];
    fgets(str, 1024, fpr);

    //convert string to integer
    otherPrId = atoi(str);
    
    return true;
}

void writeFile(const char *fileLocation, char text[]){
    FILE *fpw;
    //printf("writeFile char text");
    fpw = fopen(fileLocation, "w");
    fputs(text, fpw);
    fclose(fpw);
}

void writeFile(const char *fileLocation, long pid){
    FILE *fpw;
    //printf("writeFile long pid %ld", pid);
    fpw = fopen(chatFileLocation, "w");
    fprintf(fpw, "%ld", pid);
    fclose(fpw);
}


void sigint(int sig){
    remove(chatFileLocation);
    printf("Terminated.\n %s is deleted.", chatFileLocation);
    fflush(stdout);
    exit(0);
}

void sigusr(int sig){
    
    //check if we did not store other process's pid
    if(otherPrIdflag == false){
        //if file does not exist, just create one and write own pid.
        //if file exists assume the file contains other process's pid.
        if(!readFileExtractPid(chatFileLocation)){
            return writeFile(chatFileLocation, (long)getpid());
        }
        otherPrIdflag = true;
        writeFile(chatFileLocation, (long)getpid());
        if(type == 1){
            kill(otherPrId, SIGUSR2);
        }else if(type == 2){
            kill(otherPrId, SIGUSR1);
        }else{
            printf("Internal error. Empty type variable.");
            fflush(stdout);
            exit(0);
        }
        
        return;
    }

    if(!readFileByLine(chatFileLocation)){
        return;
    }

    //get input from user
    char input[1024];
    printf("Enter: ");
    fflush(stdout);
    fgets(input, sizeof input, stdin);
            
    //write the input recived to the shared file
    writeFile(chatFileLocation, input);

    //reallocate memory for line array
    memset(input, 0, sizeof input);

    //signal the other process
    if(type == 1){
        kill(otherPrId, SIGUSR2);
    }else if(type == 2){
        kill(otherPrId, SIGUSR1);
    }else{
        printf("Internal error. Empty type variable.");
        fflush(stdout);
        exit(0);
    }
        
}