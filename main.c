#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>

#define DELAY 500000
#define MIN_WIDTH 32
#define MIN_HEIGHT 15

volatile sig_atomic_t g_resize_requested = 0;
volatile sig_atomic_t g_stop_requested = 0;

typedef struct {
    int *values;
    int size;
    int capacity;
    int max_val;
    int min_val;
} t_stack;

t_stack g_a;
t_stack g_b;

void handle_sigint(int sig) {
    (void)sig;
    g_stop_requested = 1;
}

void handle_winch(int sig) {
    (void)sig;
    g_resize_requested = 1;
}

void init_stack(t_stack *s, int capacity) {
    s->values = (int *)malloc(sizeof(int) * capacity);
    s->size = 0;
    s->capacity = capacity;
    s->max_val = INT_MIN;
    s->min_val = INT_MAX;
}

void parse_and_add_values(char *str) {
    char *token = strtok(str, " \t\n");
    while (token != NULL) {
        int val = atoi(token);
        if (g_a.size >= g_a.capacity) {
            g_a.capacity *= 2;
            g_a.values = realloc(g_a.values, sizeof(int) * g_a.capacity);
        }
        g_a.values[g_a.size] = val;
        g_a.size++;
        if (val > g_a.max_val) g_a.max_val = val;
        if (val < g_a.min_val) g_a.min_val = val;
        token = strtok(NULL, " \t\n");
    }
}

void swap(t_stack *s) {
    if (s->size < 2) return;
    int tmp = s->values[0];
    s->values[0] = s->values[1];
    s->values[1] = tmp;
}

void push(t_stack *src, t_stack *dest) {
    if (src->size == 0) return;
    int val = src->values[0];
    for (int i = 0; i < src->size - 1; i++) {
        src->values[i] = src->values[i + 1];
    }
    src->size--;
    for (int i = dest->size; i > 0; i--) {
        dest->values[i] = dest->values[i - 1];
    }
    dest->values[0] = val;
    dest->size++;
}

void rotate(t_stack *s) {
    if (s->size < 2) return;
    int tmp = s->values[0];
    for (int i = 0; i < s->size - 1; i++) {
        s->values[i] = s->values[i + 1];
    }
    s->values[s->size - 1] = tmp;
}

void rev_rotate(t_stack *s) {
    if (s->size < 2) return;
    int tmp = s->values[s->size - 1];
    for (int i = s->size - 1; i > 0; i--) {
        s->values[i] = s->values[i - 1];
    }
    s->values[0] = tmp;
}

void exec_cmd(char *cmd) {
    if (!strcmp(cmd, "sa")) swap(&g_a);
    else if (!strcmp(cmd, "sb")) swap(&g_b);
    else if (!strcmp(cmd, "ss")) { swap(&g_a); swap(&g_b); }
    else if (!strcmp(cmd, "pa")) push(&g_b, &g_a);
    else if (!strcmp(cmd, "pb")) push(&g_a, &g_b);
    else if (!strcmp(cmd, "ra")) rotate(&g_a);
    else if (!strcmp(cmd, "rb")) rotate(&g_b);
    else if (!strcmp(cmd, "rr")) { rotate(&g_a); rotate(&g_b); }
    else if (!strcmp(cmd, "rra")) rev_rotate(&g_a);
    else if (!strcmp(cmd, "rrb")) rev_rotate(&g_b);
    else if (!strcmp(cmd, "rrr")) { rev_rotate(&g_a); rev_rotate(&g_b); }
}

void draw_bar(int y, int x, int val, int max_width, int min, int max, int color_pair) {
    long range = (long)max - (long)min;
    if (range == 0) range = 1;
    
    int len = (int)(((long)val - (long)min) * (long)max_width / range);
    if (len == 0) len = 1;

    attron(COLOR_PAIR(color_pair));
    mvprintw(y, x, "%d ", val);
    for (int i = 0; i < len; i++) {
        addch('|');
    }
    attroff(COLOR_PAIR(color_pair));
}

void render_screen(char *last_cmd, int waiting_start) {
    clear();
    int max_h, max_w;
    getmaxyx(stdscr, max_h, max_w);

    if (max_h < MIN_HEIGHT || max_w < MIN_WIDTH) {
        mvprintw(max_h / 2, (max_w - 20) / 2, "Window too small!");
        refresh();
        return;
    }

    if (waiting_start) {
        mvprintw(0, 2, "Visualizer - Press SPACE to Start");
    } else {
        mvprintw(0, 2, "Visualizer - Last Command: %s", last_cmd);
    }
    mvhline(1, 0, '-', max_w);

    int col_width = max_w / 2;
    int bar_width = col_width - 10;
    
    int global_max = (g_a.max_val > g_b.max_val) ? g_a.max_val : g_b.max_val;
    int global_min = (g_a.min_val < g_b.min_val) ? g_a.min_val : g_b.min_val;
    if (g_b.size == 0) {
        global_max = g_a.max_val;
        global_min = g_a.min_val;
    }

    mvprintw(2, 2, "Stack A (%d)", g_a.size);
    for (int i = 0; i < g_a.size; i++) {
        if (3 + i >= max_h) break;
        draw_bar(3 + i, 2, g_a.values[i], bar_width, global_min, global_max, 1);
    }

    mvprintw(2, col_width + 2, "Stack B (%d)", g_b.size);
    for (int i = 0; i < g_b.size; i++) {
        if (3 + i >= max_h) break;
        draw_bar(3 + i, col_width + 2, g_b.values[i], bar_width, global_min, global_max, 2);
    }
    refresh();
}

int read_command_nonblocking(char *buffer, int max_len) {
    static char buf[100];
    static int pos = 0;
    char c;
    
    // 標準入力（パイプ）から読み込み
    int n = read(STDIN_FILENO, &c, 1);
    
    if (n > 0) {
        if (isspace(c)) {
            if (pos > 0) {
                buf[pos] = '\0';
                strncpy(buffer, buf, max_len);
                pos = 0;
                return 1;
            }
        } else {
            if (pos < 99) {
                buf[pos++] = c;
            }
        }
    } else if (n == 0) {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <num1> <num2> ...\n", argv[0]);
        return 1;
    }

    int initial_capacity = argc * 2;
    init_stack(&g_a, initial_capacity);
    init_stack(&g_b, initial_capacity);

    for (int i = 1; i < argc; i++) {
        char *arg_copy = strdup(argv[i]);
        if (arg_copy) {
            parse_and_add_values(arg_copy);
            free(arg_copy);
        }
    }

    initscr();
    start_color();
    use_default_colors();
    curs_set(0);
    noecho();
    init_pair(1, COLOR_GREEN, -1);
    init_pair(2, COLOR_CYAN, -1);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_winch;
    sigaction(SIGWINCH, &sa, NULL);
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    // --- キーボードデバイスを直接開く ---
    FILE *f_tty = fopen("/dev/tty", "r");
    if (!f_tty) {
        endwin();
        fprintf(stderr, "Error: Could not open /dev/tty\n");
        return 1;
    }
    int tty_fd = fileno(f_tty);
    
    // TTYをノンブロッキングに設定
    int tty_flags = fcntl(tty_fd, F_GETFL, 0);
    fcntl(tty_fd, F_SETFL, tty_flags | O_NONBLOCK);

    char last_cmd[10] = "START";
    
    // --- 開始待機ループ ---
    while (!g_stop_requested) {
        if (g_resize_requested) {
            endwin(); refresh(); clear(); g_resize_requested = 0;
        }
        
        render_screen(last_cmd, 1); 

        // getch() ではなく /dev/tty から直接読む
        char ch;
        int n = read(tty_fd, &ch, 1);
        if (n > 0 && ch == ' ') {
            break;
        }
        usleep(50000);
    }
    
    // 使い終わったTTYは閉じる（またはこのままにしておく）
    fclose(f_tty);

    // --- メイン実行ループ ---
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    int pipe_open = 1;

    while (!g_stop_requested) {
        if (g_resize_requested) {
            endwin(); refresh(); clear();
            render_screen(last_cmd, 0);
            g_resize_requested = 0;
        }

        if (pipe_open) {
            char cmd_buf[20];
            int res = read_command_nonblocking(cmd_buf, 19);
            
            if (res == 1) {
                exec_cmd(cmd_buf);
                strncpy(last_cmd, cmd_buf, 9);
                render_screen(last_cmd, 0);
                usleep(DELAY);
            } else if (res == -1) {
                pipe_open = 0;
                render_screen("DONE", 0);
            }
        } else {
            usleep(100000);
        }
    }

    endwin();
    free(g_a.values);
    free(g_b.values);

    return 0;
}
