#include "mutual.h"

#define VALID_ATIS "ATIS"
#define VALID_LGT 4
#define MAX_TRY 10

void pilotCleanup(int, int, int);

void pilotCleanup(int server, int outServer, int status){
    if (server != -1){
        if (close(server) == -1) {
        	fprintf(stderr, "\033[22;31mERROR : Impossible de fermer le descripteur du fichier d'input n°%d.\n\033[00;0m", server);
            exit(status);
        }
    }
    if (outServer != -1) {
        if (close(outServer) == -1) {
        	fprintf(stderr, "\033[22;31mERROR : Impossible de fermer le descripteur du fichier d'output n°%d.\n\033[00;0m", outServer);
            exit(status);
        }
    }
    exit(EXIT_SUCCESS);
}

int main(void) {
    int server = -1;
    int outServer = -1;
    int notReceived = -1;
    int totalNak = 0;

    char request[100];
    char buf[100];
    char response[100];
    int  responseSize = 0;

    memcpy(request, PILOT_REQUEST, 100);
    if((server = open(FIFO_FILE, O_WRONLY)) == -1) {
    	fprintf(stderr, "\033[22;31mERROR : Le serveur semble ne pas être lancé.\n\033[00;0m");
    } else {
        printf("Envoi de la requète.\n");
        if (write(server, request, sizeof(request)) == -1) {
        	fprintf(stderr, "\033[22;31mERROR : Impossible d'écrire le message.\n\033[00;0m");
            pilotCleanup(server, outServer, -1);
        } else {
            if ((outServer = open(FIFO_FILE_OUT, O_RDONLY)) == -1) {
            	fprintf(stderr, "\033[22;31mERROR : Impossible d'ouvrir le fichier d'output.\n\033[00;0m");
                pilotCleanup(server, outServer, -1);
            } else {
                while (notReceived) {
                    responseSize = (int)read(outServer, buf, sizeof(buf));
                    if (responseSize == -1) {
                    	fprintf(stderr, "\033[22;31mERROR : Impossible de lire la réponse du serveur.\n\033[00;0m");
                        pilotCleanup(server, outServer, -1);
                    } else {
                        memcpy(response, buf, responseSize);
                        if (memcmp(response, VALID_ATIS, VALID_LGT) == 0) {
                            printf("Réponse du serveur : %s\n", response);
                            if (write(server, "ACK", strlen("ACK")) == -1) {
                            	fprintf(stderr, "\033[22;31mERROR : Impossible d'envoyer l'ACK\n\033[00;0m");
                            }
                            notReceived = 0;
                        } else {
                            if (totalNak == MAX_TRY) {
                            	fprintf(stderr, "\033[22;31mERROR : Le serveur ne réponds plus..Sortie\n\033[00;0m");
                                pilotCleanup(server, outServer, EXIT_FAILURE);
                            } else {
                            	fprintf(stderr, "\033[22;31mERROR : %s\n\033[00;0m", response);
                                printf("Envoi d'une requète NAK au serveur, pour un nouveau message ATIS.\n");
                                if (write(server, "NAK", strlen("NAK")) == -1) {
                                	fprintf(stderr, "\033[22;31mERROR : Impossible d'envoyer la requète NAK.\n\033[00;0m");
                                }
                                totalNak++;
                                sleep(1);
                            }
                        }
                    }
                }
            }
        }
        pilotCleanup(server, outServer, EXIT_SUCCESS);
    }
}
