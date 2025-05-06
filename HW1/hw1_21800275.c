#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define READ_END 0
#define WRITE_END 1

int main(int argc, char *argv[]) {
    char read_msg[BUFFER_SIZE];
    pid_t pid;
    int ret;
    int fd[2];
    char **new_argv;
    char *ptr = argv[1];
    int count = 0;
    int max_strlen = 0;
    int num = 0;
    int flex_mode = 0;

    // Check argument count
    if (argc < 3) {
        fprintf(stderr, "Usage: cpu <command> <search_word> [-f]\n");
        exit(1);
    }

    // Parse flexible mode flag (-f or -F)
    if (argc >= 4 && argv[3][0] == '-' && toupper(argv[3][1]) == 'F' && argv[3][2] == '\0')
        flex_mode = 1;

    // Calculate max token length for memory allocation
    while (*ptr != '\0') {
        if (*ptr == ' ') {
            count++;
            if (num > max_strlen) max_strlen = num;
            num = 0;
        } else {
            num++;
        }
        ptr++;
    }
    if (num > max_strlen) max_strlen = num;

    // Allocate memory for argument tokens (count+2 entries)
    new_argv = malloc(sizeof(char *) * (count + 2));
    for (int i = 0; i <= count; i++)
        new_argv[i] = malloc(sizeof(char) * (max_strlen + 1));

    // Tokenize the command string into new_argv
    ptr = argv[1];
    int i = 0;
    int j = 0;
    while (*ptr != '\0') {
        if (*ptr != ' ') {
            new_argv[i][j++] = *ptr;
        } else {
            new_argv[i][j] = '\0';
            i++;
            j = 0;
        }
        ptr++;
    }
    new_argv[i][j] = '\0';
    new_argv[i + 1] = NULL;

    // Create a pipe for inter-process communication
    if (pipe(fd) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    // Fork a child process
    pid = fork();
    if (pid == 0) {
        // Child: redirect stdout to pipe and execute command
        close(fd[READ_END]);
        dup2(fd[WRITE_END], STDOUT_FILENO);
        close(fd[WRITE_END]);
        ret = execvp(new_argv[0], new_argv);
        if (ret < 0) {
            perror("Error: execvp failed");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        // Parent: read from pipe and process output
        close(fd[WRITE_END]);
        wait(NULL);

        FILE *fd_read = fdopen(fd[READ_END], "r");
        int line_no = 1;
        int search_len = strlen(argv[2]);

        // Read each line from the pipe
        while (fgets(read_msg, BUFFER_SIZE, fd_read)) {
            int match_flag = 0;
            int grep_point[256] = {0};         // Positions for Default match mode
            int grep_char[BUFFER_SIZE] = {0};  // Positions for flexible mode
            int l = 0;
            int msg_len = strlen(read_msg);

            if (flex_mode) {
                // Flexible mode: highlight any matching character
                for (int j = 0; j < msg_len; j++) {
                    for (int k = 0; k < search_len; k++) {
                        if (read_msg[j] == argv[2][k]) {
                            match_flag = 1;
                            if (l < BUFFER_SIZE - 1) {
                                grep_char[l++] = j;
                                grep_char[l] = -1;
                                break;
                            } else {
                                fprintf(stderr, "Buffer overflow in flexible mode\n");
                                return 1;
                            }
                        }
                    }
                }
            } else {
                // Default match mode: find substring positions
                if (msg_len >= search_len) {
                    for (int j = 0; j <= msg_len - search_len; j++) {
                        for (int k = 0; k < search_len; k++) {
                            if (read_msg[j + k] != argv[2][k]) break;
                            if (k == search_len - 1) {
                                match_flag = 1;
                                if (l < 256 - 1) {
                                    grep_point[l++] = j;
                                    grep_point[l] = -1;
                                } else {
                                    fprintf(stderr, "Buffer overflow in Default mode\n");
                                    return 1;
                                }
                                j += search_len - 1;
                            }
                        }
                    }
                }
            }

            // Print highlighted line if a match was found
            if (match_flag) {
                printf("\033[0;32m[%d]\033[0m ", line_no);  // Line number in green
                l = 0;
                for (int k = 0; k < msg_len; k++) {
                    if (flex_mode) {
                        if (grep_char[l] != -1 && grep_char[l] == k) {
                            printf("\033[0;31m%c\033[0m", read_msg[k]);  // Highlight char red
                            l++;
                        } else {
                            putchar(read_msg[k]);
                        }
                    } else {
                        if (grep_point[l] != -1 && grep_point[l] == k)
                            printf("\033[0;31m");  // Start red highlight
                        putchar(read_msg[k]);
                        if (grep_point[l] != -1 && grep_point[l] + search_len - 1 == k) {
                            printf("\033[0m");  // End red highlight
                            l++;
                        }
                    }
                }
            }
            line_no++;
        }
        printf("\033[0m");  // Restore default terminal colors after highlighting

        // Cleanup
        close(fd[READ_END]);
        fclose(fd_read);
    } else {
        perror("Fork failed");
        return 1;
    }

    // Free allocated memory
    for (int idx = 0; idx <= count; idx++)
        free(new_argv[idx]);
    free(new_argv);

    return 0;
}