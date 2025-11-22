#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1; // Добавляем переменную для таймаута
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0}, // Добавляем опцию timeout
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4: // Обработка timeout
            timeout = atoi(optarg);
            if (timeout <= 0) {
                printf("timeout must be a positive number\n");
                return 1;
            }
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  // Создаем pipes для каждого процесса (если не используем файлы)
  int pipes[pnum][2];
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        printf("Pipe creation failed!\n");
        return 1;
      }
    }
  }

  int active_child_processes = 0;
  pid_t child_pids[pnum]; // Массив для хранения PID дочерних процессов
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      child_pids[i] = child_pid; // Сохраняем PID дочернего процесса
      if (child_pid == 0) {
        // child process
        // диапазон для текущего процесса
        int segment_size = array_size / pnum;
        int begin = i * segment_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * segment_size;
        
        // min/max в своем сегменте
        struct MinMax local_min_max = GetMinMax(array, begin, end);
        
        if (with_files) {
          // файлы для передачи данных
          char filename[30];
          sprintf(filename, "min_max_%d.txt", i);
          FILE *file = fopen(filename, "w");
          if (file != NULL) {
            fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
            fclose(file);
          }
        } else {
          // pipe для передачи данных
          close(pipes[i][0]); // Закрываем чтение в дочернем процессе
          write(pipes[i][1], &local_min_max, sizeof(local_min_max));
          close(pipes[i][1]);
        }
        
        free(array);
        return 0;
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Если задан таймаут, используем waitpid с таймаутом
  if (timeout > 0) {
    int remaining_children = active_child_processes;
    int waited_time = 0;
    
    while (remaining_children > 0 && waited_time < timeout) {
      sleep(1); // Ждем 1 секунду
      waited_time++;
      
      // Проверяем завершились ли какие-то процессы
      pid_t finished_pid;
      while ((finished_pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        remaining_children--;
      }
    }
    
    // Если время вышло и остались незавершенные процессы
    if (remaining_children > 0) {
      printf("Timeout reached (%d seconds), killing child processes\n", timeout);
      
      // Отправляем SIGKILL всем дочерним процессам
      for (int i = 0; i < pnum; i++) {
        // Проверяем, существует ли еще процесс
        if (kill(child_pids[i], 0) == 0) { // Проверка существования процесса
          kill(child_pids[i], SIGKILL);
        }
      }
      
      // Ждем завершения всех процессов после отправки SIGKILL
      while (wait(NULL) > 0) {
        // Ждем завершения всех дочерних процессов
      }
      
      printf("Child processes terminated by timeout\n");
      free(array);
      return 1;
    }
  } else {
    // Если таймаут не задан, ждем все процессы как раньше
    while (active_child_processes > 0) {
      wait(NULL);
      active_child_processes -= 1;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Собираем результаты только от завершившихся процессов
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    // Проверяем, был ли процесс завершен по таймауту
    if (timeout > 0) {
      int status;
      pid_t result = waitpid(child_pids[i], &status, WNOHANG);
      if (result == 0) {
        // Процесс все еще работает (должен был быть убит)
        continue;
      }
    }

    if (with_files) {
      // Чтение из файлов
      char filename[30];
      sprintf(filename, "min_max_%d.txt", i);
      FILE *file = fopen(filename, "r");
      if (file != NULL) {
        fscanf(file, "%d %d", &min, &max);
        fclose(file);
        remove(filename); 
      }
    } else {
      // Чтение из pipes
      close(pipes[i][1]); // Закрываем запись в родительском процессе
      struct MinMax local_min_max;
      if (read(pipes[i][0], &local_min_max, sizeof(local_min_max)) > 0) {
        min = local_min_max.min;
        max = local_min_max.max;
      }
      close(pipes[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}