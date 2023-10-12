#include <ctype.h>
#include <dirent.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct process {
  int pid;
  char* user;
  char name[256];
  char state;
};

void* process_list(void* args);

void* send_signal(void* args);

int is_process(char* process);

struct process populate_process(int pid, char* user, char name[], char state);

void print_process_table(struct process* p, int proc_count);

int main(int argc, char* argv[]) {
  pthread_t proc_tid, sig_tid;

  while (1) {
    pthread_create(&proc_tid, NULL, &process_list, NULL);
    pthread_create(&sig_tid, NULL, &send_signal, NULL);

    pthread_join(sig_tid, NULL);

    sleep(1);
  }

  return 0;
}

void* process_list(void* args) {
  DIR* proc_dir = opendir("/proc");

  if (proc_dir == NULL) {
    perror("Falha ao acessar a pasta de processis /proc");
    pthread_exit(NULL);
  }

  struct dirent* entry;
  int proc_count = 0;
  struct process* p = malloc(sizeof(struct process) * 20);

  while ((entry = readdir(proc_dir)) != NULL && proc_count < 20) {
    if (is_process(entry->d_name)) {
      char buf[256];
      snprintf(buf, sizeof(buf), "/proc/%d/stat", atoi(entry->d_name));

      struct stat st;
      stat(buf, &st);

      struct passwd* pw = getpwuid(st.st_uid);
      if (pw == NULL) continue;

      char* user = pw->pw_name;

      FILE* f = fopen(buf, "r");
      if (f == NULL) {
        perror("Falha ao acessar o arquivo de status de um processo");
        pthread_exit(NULL);
      }

      int pid;
      char name[256], state;

      fscanf(f, "%d (%256[^)]) %c", &pid, name, &state);

      p[proc_count] = populate_process(pid, user, name, state);

      fclose(f);

      proc_count++;
    }
  }

  print_process_table(p, proc_count);

  closedir(proc_dir);

  free(p);
}

void* send_signal(void* args) {
  int pid, signal;

  scanf("%d %d", &pid, &signal);

  int sig_st = kill(pid, signal);

  if (sig_st != 0) {
    perror("Falha ao matar esse processo");
    pthread_exit(NULL);
  }
}

int is_process(char* process) {
  while (*process != '\0') {
    if (*process < '0' || *process > '9') {
      return 0;
    }

    process++;
  }

  return 1;
}

struct process populate_process(int pid, char* user, char name[], char state) {
  struct process p;

  p.pid = pid;
  p.user = user;
  strcpy(p.name, name);
  p.state = state;

  return p;
}

void print_process_table(struct process* p, int proc_count) {
  printf(
      "PID      | User                     | PROCNAME                          "
      "         "
      "| Estado "
      "|\n");
  printf(
      "---------|--------------------------|-----------------------------------"
      "---------|"
      "--------"
      "|\n");

  for (int i = 0; i < proc_count; i++) {
    printf("%-9d| %-24s | %-42s | %-6c |\n", p[i].pid, p[i].user, p[i].name,
           p[i].state);
  }
}