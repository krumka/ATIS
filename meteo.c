#include "mutual.h"

static int meteo = -1;
static int lock =-1;
static bool cont = true;

static void openLock(void);
static void openMeteo(void);
static void closeLock(void);
static void closeMeteo(void);
static void pilotCleanup(int);
static int genAtis(void);

static char ATIS[5][100] = {
    "ATIS 1ONE EBLG 1803 00000KT 0600 FG OVC008 BKN040 PROB40 2024 0300 DZ FG OVC002 BKN040\0",
    "ATIS 2TOW EBBR 0615 20015KT 8000 RA SCT010 OVC015 TEMPO 0608 5000 RA BKN005 BECMG 0810 NSW BKN025\0",
    "ATIS 3TRHE METAR VHHH 231830Z 06008KT 7000 FEW010SCT022 20/17 Q1017 NOSIG 5000 RA BKN005\0",
    "ATIS 4FRUO 20015KT 8000 RA SCT010 OVC015 2024 0300 DZ FG 0810 9999 NSW BKN025\0",
    "ATIS 5VEFI KT 7000 FEW010SCT02 EMPO 0608 5000 RA BKN005 EMPO 0608 5000 RA BKN005\0"
};

static void openLock(void){
	if ((lock = open(FICHIERLOCK, O_CREAT | O_WRONLY , S_IRUSR | S_IWUSR | S_IRGRP)) == -1){
		printf("Impossible de créer le fichier lock.\n");
		exit(EXIT_FAILURE);
	}
}

static void openMeteo(void){
	if((meteo = open(FICHIERMETEO, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP)) == -1){
		printf("Impossible d'ouvrir ou de créer le fichier météo.\n");
		exit(EXIT_FAILURE);
	}
}

static void closeLock(void){
    if (FICHIERLOCK){
        if (close(lock) == -1){
            printf("Impossible de fermer le fichier lock.\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void closeMeteo(void){
    if (FICHIERMETEO){
        if (close(meteo) == -1){
            printf("Impossible de fermer le fichier météo : %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

static int generateAtis(void){
	while(cont){
		sleep(5);
		unsigned int msg = 0;
		srand(time(NULL));
		msg = rand()%5;
		openLock();
		unlink(FICHIERMETEO);
		sleep(2);
		openMeteo();
		if(write(meteo, ATIS[msg], strlen(ATIS[msg])) == -1){
			return EXIT_FAILURE;
		}
		printf("Nouveau message ATIS : %s\n", ATIS[msg]);
        closeLock();
        sleep(1);
        unlink(FICHIERLOCK);
	}
	return EXIT_SUCCESS;
}

void last(int state){
	if(state == SIGINT){
		cont = false;
		printf("Sortie du programme.\n");
	}
}

int main(){
	signal(SIGINT, last);
	return generateAtis();
	return 0;
}
