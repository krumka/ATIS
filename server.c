#include "mutual.h"

int input = -1;
int output = -1;
bool listen = true;
int nb = 0;
char **requests = NULL;

int atis(char *);
void createAndOpenFifos(void);
void gen(void);
void exitWrong(char *);
void clean(int);
void * xrealloc(void *, size_t, size_t);
bool doesFileExists(const char *);

bool doesFileExists(const char * pathName){
	struct stat         info;
    return (stat(pathName, &info) == 0);
}

void clean(int state){
	printf("\nNettoyage..");
    if (input != -1 || output != -1) {
        if (close(input) == -1) {
            fprintf(stderr, "\033[22;31mERROR : Impossible de fermer le descripteur du fichier d'input n°%d.\n\033[00;0m", input);
        }
        if (close(output) == -1) {
            fprintf(stderr, "\033[22;31mERROR : Impossible de fermer le descripteur du fichier d'output n°%d.\n\033[00;0m", output);
        }
    }
    unlink(FIFO_FILE);
    unlink(FIFO_FILE_OUT);
    for(int i = 0; i < nb; i++){
        if(requests[i] != NULL){
            free(requests[i]);
            requests[i] = NULL;
        }
    }
    if(requests != NULL){
        free(requests);
        requests = NULL;
    }
    sleep(1);
    printf("Fait!\n");
    if(state == 0 || state == SIGINT){
    	exit(0);
    }else if(state == -1){
    	exit(EXIT_FAILURE);
    }else{
    	exit(2);
    }
}

void exitWrong(char * error){
	fprintf(stderr, "\033[22;31mERROR : %s\n\033[00;0m", error);
	clean(-1);
}

void createAndOpenFifos(void){
	if(mkfifo(FIFO_FILE, S_IRUSR | S_IWUSR) == -1){
		exitWrong("Impossible de créer l'input du fifo.\n");
	}else{
		if ((input = open(FIFO_FILE, O_RDWR)) == -1) {
        	exitWrong("Impossible d'ouvrir le serveur fifo.\n");
    	}
	}
	if(mkfifo(FIFO_FILE_OUT, S_IRUSR | S_IWUSR) == -1){
		exitWrong("Impossible de créer l'output du fifo.\n");
	}else{
		if ((output = open(FIFO_FILE_OUT, O_RDWR)) == -1) {
        	exitWrong("Impossible d'ouvrir le serveur d'output fifo.\n");
    	}
	}
}

int atis(char * atisMsg){
	int fichierMeteo = 0;
	int atisSize = 0;
	char buf[100];
	fichierMeteo = open(FICHIERMETEO, O_RDONLY);
	if(doesFileExists(FICHIERLOCK)){
        char * busy = "Le serveur météo est occupé veuillez réessayer.";
        printf("%s\n", busy);
        memcpy(atisMsg, busy, strlen(busy));
        atisSize = strlen(busy);
    }else if(fichierMeteo == -1){
		char * unreachable = "Le serveur météo ne répond pas.";
		printf("Impossible d'ouvrir le fichier météo.\n");
		memcpy(atisMsg, unreachable, strlen(unreachable));
		atisSize = strlen(unreachable);
	}else{
		atisSize = (int)read(fichierMeteo, buf, sizeof(buf));
		if(atisSize == -1){
			exitWrong("Impossible de lire le fichier météo.");
		}else{
			memcpy(atisMsg, buf, atisSize);
            char *token;
            token = strtok(atisMsg, "\0");
            atisMsg = token;
            atisSize = strlen(atisMsg);
            printf("%s\n", token);
            printf("%d\n", atisSize);
		}
		return atisSize;
	}
}

void * xrealloc(void *ptr, size_t nmemb, size_t size) {
    void *new_ptr;
    size_t new_size = nmemb * size;

    if(ptr == NULL){
        new_ptr = malloc(new_size);
    }else{
    	new_ptr = realloc(ptr, new_size);
    }
    if (new_ptr == NULL){
    	printf("xrealloc: out of memory (new_size %zu bytes)", new_size);
    }
    return new_ptr;
}

void gen(void){
	struct pollfd       fd[] = {
        { input, POLLIN | POLLHUP, 0 }
    };
    char requestPacket[100];
    requests = NULL;
    char atisMsg[100];
    printf("Press ^C to exit\nListening..\n");
    while(listen){
    	int fifoActions = poll(fd, 1, 1000);
    	if(fifoActions == -1){
    		if(listen){
    			printf("Nouveau Polling.\n");
    		}
    	}else if(fifoActions == 0){
    		printf("Listening..\n");
    	}else{
    		int packet = (int)read(input, requestPacket, 100);
    		if(packet == -1){
    			if(fd[0].revents & POLLHUP){
    				close(input);
    				if((input = open(FIFO_FILE, O_RDWR)) == -1){
    					exitWrong("Impossible d'ouvrir l'input fifo.\n");
    				}
    			}
    		}else{
    			requests = xrealloc(requests, (size_t)nb+1, 100);
    			requests[nb] = malloc(100);
    			memcpy(requests[nb], requestPacket, packet);
    			if ((memcmp(requests[nb], PILOT_REQUEST, strlen(PILOT_REQUEST)) == 0) || (memcmp(requests[nb], "NAK", strlen("NAK")) == 0)){
    				printf("Requète n°%d reçue.\n", nb);
    				size_t tailleMsg = (size_t)atis(atisMsg);
    				printf("Envoi du message ATIS \"%s\" à %d\n", atisMsg, nb);
    				size_t written_b = 0;
    				if ((written_b = write(output, atisMsg, tailleMsg)) == -1) {
                        exitWrong("Impossible d'envoyer le message.\n");
                    }
                    if(written_b != tailleMsg){
                    	exitWrong("Le message envoyé ne correspond pas au message lu.\n");
                    }
                    nb++;
    			}else if(memcmp(requests[nb], "ACK", strlen("ACK")) == 0){
    				printf("\033[22;32mUn pilote vient de recevoir le message ATIS.\n\033[00;0m");
    			}else{
    				exitWrong("Aucun message valide intercepté.\n");
    			}
    		}
    	}
    }
}

int main(){
	signal(SIGINT, clean);
	createAndOpenFifos();
	gen();
	clean(0);
	return 0;
}