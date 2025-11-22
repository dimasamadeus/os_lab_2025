#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    printf("Создаем зомби-процесс...\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс: PID = %d\n", getpid());
        printf("Дочерний процесс завершается и становится зомби!\n");
        exit(0);
    } else {
        // Родительский процесс
        printf("Родительский процесс: PID = %d\n", getpid());
        printf("Дочерний процесс создан с PID = %d\n", pid);
        printf("Родитель НЕ вызывает wait() и спит 30 секунд...\n");
        
        sleep(30);
        
        printf("Родительский процесс завершается\n");
    }
    
    return 0;
}