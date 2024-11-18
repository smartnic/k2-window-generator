#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG

#define INSN_CLASS_MASK 0x07

#define BPF_JMP 0x05
#define BPF_JMP32 0x06

#define BPF_LD 0x00
#define BPF_LDX 0x01
#define BPF_ST 0x02
#define BPF_STX 0x03

#define MIN_WINDOW_SIZE 5
#define MAX_WINDOW_SIZE 10

struct window {
    int l;
    int r;
    int heuristic_cost;
    struct window *next;
};

unsigned char bitmap_get(unsigned char bitmap[], size_t n) {
    size_t a = n / 8;
    size_t b = n % 8;
    return (bitmap[a] >> b) & 1;
}

void bitmap_set(unsigned char bitmap[], size_t n) {
    size_t a = n / 8;
    size_t b = n % 8;
    bitmap[a] |= 1 << b;
}

void bitmap_clear(unsigned char bitmap[], size_t n) {
    size_t a = n / 8;
    size_t b = n % 8;
    bitmap[a] &= ~(1 << b);
}

struct window *split(struct window *head) {
    struct window *fast = head;
    struct window *slow = head;

    while (fast != NULL && fast->next != NULL) {
        fast = fast->next->next;

        if (fast != NULL) {
            slow = slow->next;
        }
    }

    struct window* temp = slow->next;
    slow->next = NULL;
    return temp;
}

struct window* merge(struct window* first, struct window* second) {
    if (first == NULL) return second;
    if (second == NULL) return first;

    if (first->heuristic_cost >= second->heuristic_cost) {
        first->next = merge(first->next, second);
        return first;
    } else {
        second->next = merge(first, second->next);
        return second;
    }
}

struct window* merge_sort(struct window* head) {
    if (head == NULL || head->next == NULL) {
        return head;
    }

    struct window* second = split(head);
    head = merge_sort(head);
    second = merge_sort(second);
    return merge(head, second);
}

int main(int argc, char *argv[])
{
    // has optional arg of max_windows; otherwise uses default
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "USAGE: %s </path/to/insns.txt>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int max_windows = -1;

    if (argc == 3) {
        max_windows = atoi(argv[2]);

        if (max_windows == 0) {
            max_windows = -1;
        }
    }

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t n = 0;
    size_t num_insns = 0;

    while (getline(&line, &n, file) > 0) {
        ++num_insns;
    }

    // if no insns, then nothing to do.
    if (num_insns == 0) {
        fclose(file);
        return EXIT_FAILURE;
    }

    size_t bitmap_size = (num_insns + 7) / 8;
    int cost_map[num_insns]; // Populate with prefix traversal
    memset(cost_map, 0, num_insns);
    num_insns = 0;
    unsigned char bitmap[bitmap_size];
    memset(bitmap, 0, bitmap_size);
    ssize_t len = 0;
    rewind(file);

    while ((len = getline(&line, &n, file)) > 0) {
        if (line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        unsigned char code, src_reg, dst_reg;
        signed short off;
        signed int imm;
        
        if (sscanf(line, "{%hhu %hhu %hhu %hd %d}", &code, &src_reg, &dst_reg, &off, &imm) != 5) {
            fprintf(stderr, "INVALID INSTRUCTION FORMAT: \"%s\"\n", line);
            if (num_insns > 0) {
                cost_map[num_insns] = cost_map[num_insns-1];
            }
            ++num_insns;
            continue;
        }

        int insn_class = code & INSN_CLASS_MASK;
        unsigned char is_preferred = 0;

        if (insn_class == BPF_JMP || insn_class == BPF_JMP32) {
            int jmp_src = num_insns;
            int jmp_dst;

            // Positive offset
            //if (off >= 0) {
            //    jmp_dst = jmp_src + off + 1;
            //// Negative offset
            //// TODO: ensure these are handled well
            //} else {
            //    jmp_dst = jmp_src + off;
            //}

            jmp_dst = jmp_src + off + 1;

            bitmap_set(bitmap, jmp_src); // JMP_SRC
            bitmap_set(bitmap, jmp_dst); // JMP_DST
        } else if (insn_class == BPF_LD || insn_class == BPF_LDX ||
                   insn_class == BPF_ST || insn_class == BPF_STX) {
            is_preferred = 1;
        }

        if (num_insns == 0) {
            cost_map[0] = is_preferred;
        } else {
            cost_map[num_insns] = cost_map[num_insns-1] + is_preferred;
        }

        ++num_insns;
    }

    #ifdef DEBUG
    FILE *debug = fopen("debug.txt", "w");

    for (size_t i = 0; i < num_insns; ++i) {
        fprintf(debug, "%d\n", bitmap_get(bitmap, i));
    }

    fclose(debug);
    #endif

    free(line);
    fclose(file);

    size_t num_windows = 0;
    size_t left_ptr = 0, right_ptr = 0;
    struct window *head = NULL;
    struct window *tail = NULL;

    while (right_ptr < num_insns) {
        if (bitmap_get(bitmap, right_ptr) == 1) {
            if (left_ptr == right_ptr) {
                ++right_ptr;
                ++left_ptr;
                continue;
            }

            int size = right_ptr-left_ptr;

            // Window must be within the size we're interested in
            if (size >= MIN_WINDOW_SIZE && size <= MAX_WINDOW_SIZE) {
                ++num_windows;
                struct window *current;

                if (head == NULL) {
                    head = malloc(sizeof(struct window));
                    current = head;
                } else {
                    current = malloc(sizeof(struct window));
                    tail->next = current;
                }

                current->l = left_ptr;
                current->r = right_ptr-1;
                // determine cost
                if (left_ptr == 0) {
                    current->heuristic_cost = cost_map[right_ptr-1];
                } else {
                    current->heuristic_cost = cost_map[right_ptr-1] - cost_map[left_ptr-1];
                }
                ////
                tail = current;
                current->next = NULL;
            }
            
            ++right_ptr;
            left_ptr = right_ptr;
        } else {
            ++right_ptr;
        }
    }

    // add window at very end if not 1 (i.e. dangling window)
    if (bitmap_get(bitmap, right_ptr - 1) == 0) {
        int size = right_ptr-left_ptr;

        // Window must be within the size we're interested in
        if (size >= MIN_WINDOW_SIZE && size <= MAX_WINDOW_SIZE) {
            ++num_windows;
            struct window *current;

            if (head == NULL) {
                head = malloc(sizeof(struct window));
                current = head;
            } else {
                current = malloc(sizeof(struct window));
                tail->next = current;
            }

            current->l = left_ptr;
            current->r = right_ptr-1;
            // determine cost
            if (left_ptr == 0) {
                current->heuristic_cost = cost_map[right_ptr-1];
            } else {
                current->heuristic_cost = cost_map[right_ptr-1] - cost_map[left_ptr-1];
            }
            ////
            tail = current;
            current->next = NULL;
        }
    }

    if (num_windows == 0) {
        #ifdef DEBUG
        printf("NO WINDOWS FOUND\n");
        #endif
        return EXIT_FAILURE;
    }

    // cap number of windows to user-specified amount
    if (max_windows != -1 && max_windows < num_windows) {
        num_windows = max_windows;
    }

    head = merge_sort(head);
    struct window* temp = head;
    char *start_window_buf = malloc(4096);
    char *end_window_buf = malloc(4096);
    start_window_buf[0] = '\0';
    end_window_buf[0] = '\0'; 
    int i = 0;
    FILE *output = fopen("output.txt", "w");
    while (i < num_windows) {
        int l=temp->l;
        int r=temp->r;
        char l_str[32], r_str[32];
        sprintf(l_str, "%d", l);
        sprintf(r_str, "%d", r);
        strcat(start_window_buf, l_str);
        strcat(end_window_buf, r_str);
        if (i < num_windows - 1) {
            strcat(start_window_buf, ",");
            strcat(end_window_buf, ",");
        }
        temp = temp->next;
        ++i;
    }
    fprintf(output, "--win_s_list %s --win_e_list %s", start_window_buf, end_window_buf);
    free(start_window_buf);
    free(end_window_buf);
    fclose(output);
    // free linked list at the very end
    while (head != NULL) {
        #ifdef DEBUG
        printf("[%d,%d]=%d\n", head->l, head->r, head->heuristic_cost);
        #endif
        struct window *next = head->next;
        free(head);
        head = next;
    }
    return EXIT_SUCCESS;
}