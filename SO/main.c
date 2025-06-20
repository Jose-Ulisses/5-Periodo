#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAXBUFF 128
#define ROWS 5
#define COLS 5

char maze[ROWS][COLS] = {
    {' ', ' ', '#', ' ', 'E'},
    {'#', '#', '#', ' ', '#'},
    {' ', ' ', ' ', ' ', '#'},
    {' ', '#', '#', '#', '#'},
    {'S', ' ', ' ', ' ', ' '}
};

int playerRow = 4;
int playerCol = 0;

int readfd_global, writefd_global;

void client(int readfd, int writefd){
    char buff[MAXBUFF];
    while (1) {
        printf("\nComando -> ");
        fgets(buff, MAXBUFF, stdin);
        buff[strcspn(buff, "\n")] = '\0';

        write(writefd, buff, strlen(buff) + 1);

        int n = read(readfd, buff, MAXBUFF);
        if (n > 0) {
            buff[n] = '\0';
            printf("Resposta <- %s\n", buff);
            if (strcmp(buff, "EXIT") == 0) {
                printf("Parabéns! Você encontrou a saída!\n");
                break;
            }
        }
    }
}

void *server_thread(void *arg){
    char command[MAXBUFF];
    char response[MAXBUFF];

    while (1) {
        int n = read(readfd_global, command, MAXBUFF);
        if (n > 0) {
            command[n] = '\0';
            move_player(command, response);
            write(writefd_global, response, strlen(response) + 1);
            if (strcmp(response, "EXIT") == 0)
                break;
        }
    }

    return NULL;
}

int move_player(const char *command, char *response){
    int newRow = playerRow;
    int newCol = playerCol;

    if (strcmp(command, "UP") == 0) newRow--;
    else if (strcmp(command, "DOWN") == 0) newRow++;
    else if (strcmp(command, "LEFT") == 0) newCol--;
    else if (strcmp(command, "RIGHT") == 0) newCol++;
    else {
        strcpy(response, "INVALID");
        return 0;
    }

    if (newRow < 0 || newRow >= ROWS || newCol < 0 || newCol >= COLS) {
        strcpy(response, "WALL");
        return 0;
    }

    char cell = maze[newRow][newCol];

    if (cell == '#') {
        strcpy(response, "WALL");
    } else if (cell == 'E') {
        playerRow = newRow;
        playerCol = newCol;
        strcpy(response, "EXIT");
    } else {
        playerRow = newRow;
        playerCol = newCol;
        strcpy(response, "OK");
    }

    return 1;
}

int main() {
    int pipe1[2], pipe2[2], pid;

    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        perror("Erro ao criar pipes");
        exit(1);
    }

    if ((pid = fork()) < 0) {
        perror("Erro ao criar processo filho");
        exit(1);
    }

    if (pid > 0) {
        close(pipe1[0]);
        close(pipe2[1]);
        client(pipe2[0], pipe1[1]);
        close(pipe1[1]);
        close(pipe2[0]);
    } else {
        close(pipe1[1]);
        close(pipe2[0]);

        readfd_global = pipe1[0];
        writefd_global = pipe2[1];

        pthread_t tid;
        if (pthread_create(&tid, NULL, server_thread, NULL) != 0) {
            perror("Erro ao criar thread");
            exit(1);
        }
    
        pthread_join(tid, NULL);

        close(pipe1[0]);
        close(pipe2[1]);
    }

    return 0;
}