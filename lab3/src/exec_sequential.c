#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        printf("Fork failed!\n");
        return 1;
    }
    else if (pid == 0) {        
        char *args[] = {"./sequential_min_max", argv[1], argv[2], NULL};
        
        // Заменяем образ процесса на sequential_min_max
        execvp(args[0], args);
        
        // Если exec вернул управление - значит ошибка
        printf("Exec failed!\n");
        return 1;
    }
    else {
        // Родительский процесс - ждем завершения дочернего
        printf("Parent process (PID: %d) waiting for child (PID: %d)...\n", 
               getpid(), pid);
        
        int status;
        waitpid(pid, &status, 0);
    }
    
    return 0;
}