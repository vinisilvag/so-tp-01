#include <ctype.h>
#include <dirent.h>
#include <ncurses.h>
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

void* process_list(void*);

void* send_signal(void*);

int is_process(char*);

struct process populate_process(int, char*, char[], char);

void print_process_table(struct process*, int, WINDOW*);

// void clear();

int main(int argc, char* argv[]) {
  initscr();

  int height = LINES;
  int width = COLS;

  WINDOW* proc_win = newwin(22, width, 0, 0);
  WINDOW* sig_win = newwin(height - 22, width, 0, 0);

  pthread_t proc_tid, sig_tid;

  int* cond;

  pthread_create(&proc_tid, NULL, &process_list, proc_win);

  do {
    pthread_create(&sig_tid, NULL, &send_signal, sig_win);
    pthread_join(sig_tid, (void*)&cond);
  } while (cond);

  pthread_cancel(proc_tid);

  endwin();

  return 0;
}

void* process_list(void* args) {
  WINDOW* proc_win = (WINDOW*)args;

  while (1) {
    DIR* proc_dir = opendir("/proc");

    if (proc_dir == NULL) {
      perror("Falha ao acessar a pasta de processis /proc");
      return NULL;
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
          return NULL;
        }

        int pid;
        char name[256], state;

        fscanf(f, "%d (%256[^)]) %c", &pid, name, &state);

        p[proc_count] = populate_process(pid, user, name, state);

        fclose(f);

        proc_count++;
      }
    }

    print_process_table(p, proc_count, proc_win);

    closedir(proc_dir);

    free(p);

    usleep(1000000);

    wclear(proc_win);
  }
}

void* send_signal(void* args) {
  WINDOW* sig_win = (WINDOW*)args;

  char str[256];
  int pid, signal;

  mvgetstr(22, 0, str);

  sscanf(str, "%d %d", &pid, &signal);

  int sig_st = kill(pid, signal);

  if (sig_st == 0) {
    mvprintw(23, 0, "Sinal %d enviado para o PID %d\n", signal, pid);

    refresh();

    usleep(2000000);

    pthread_exit((void*)0);
  } else {
    mvprintw(23, 0, "Falha ao matar esse processo");

    refresh();

    usleep(2000000);

    wclear(sig_win);

    refresh();

    pthread_exit((void*)1);
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

void print_process_table(struct process* p, int proc_count, WINDOW* proc_win) {
  wprintw(
      proc_win,
      "PID      | User                     | PROCNAME                        "
      "  "
      "         "
      "| Estado "
      "|\n");
  wprintw(
      proc_win,
      "---------|--------------------------|-----------------------------------"
      "---------|"
      "--------"
      "|\n");

  for (int i = 0; i < proc_count; i++) {
    wprintw(proc_win, "%-9d| %-24s | %-42s | %-6c |\n", p[i].pid, p[i].user,
            p[i].name, p[i].state);
  }

  wrefresh(proc_win);
}

/*
 * uma forma alternativa limpar o terminal sem usar o comando system (cls ou
 * clear) pois o parâmetro passado é diferente entre sistemas Windows e
 * Unix-like
 */
// void clear() { printf("\033[H\033[J"); }