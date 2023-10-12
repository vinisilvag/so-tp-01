#include <ctype.h>
#include <dirent.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct process {
  int pid;
  char* user;
  char name[256];
  char state;
};

int process_list();

int send_signal();

int is_process(char* process);

struct process populate_process(int pid, char* user, char name[], char state);

void print_process_table(struct process* p, int proc_count);

int main(int argc, char* argv[]) {
  while (1) {
    int proc_st = process_list();
    if (proc_st == -1) {
      perror("Falha ao listar os processos");
      return 0;
    }

    int sig_st = send_signal();
    if (sig_st != 0) {
      perror("Falha ao matar esse processo");
      return 0;
    }
  }

  return 0;
}

int process_list() {
  DIR* proc_dir = opendir("/proc");

  if (proc_dir == NULL) return -1;

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
      if (f == NULL) return -1;

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

  return 0;
}

int send_signal() {
  int pid, signal;

  scanf("%i %i", &pid, &signal);

  int sig_st = kill(pid, signal);

  return sig_st;
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