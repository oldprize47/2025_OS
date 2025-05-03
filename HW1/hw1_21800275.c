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
    int chr_flag = 0;

    // 알규먼트 갯수 확인
    if (argc < 3) {
        fprintf(stderr, "usage: cpu <string>\n");
        exit(1);
    }

    if (argc >= 4 && toupper(argv[3][0]) == 'T') chr_flag = 1;  // for creative service

    // 메모리 할당을 위한 <command>라인의 문자열 크기 측정
    while (*ptr != 0) {
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

    // 메모리 동적 할당당
    new_argv = (char **)malloc(sizeof(char *) * (count + 2));
    for (int i = 0; i <= count; i++) *(new_argv + i) = (char *)malloc(sizeof(char) * (max_strlen + 1));

    // 커멘트 라인 토크나이즈
    ptr = argv[1];
    int i = 0;
    int j = 0;
    while (*ptr != 0) {
        if (*ptr != ' ') {
            new_argv[i][j] = *ptr;
            j++;
        } else {
            new_argv[i][j] = 0;
            i++;
            j = 0;
        }
        ptr++;
    }
    new_argv[i][j] = 0;
    new_argv[i + 1] = 0;

    /* create the pipe */
    if (pipe(fd) == -1) {
        fprintf(stderr, "Pipe failed");
        return 1;
    }

    /* now fork a child process */
    if ((pid = fork()) == 0) { /* child process */
        close(fd[READ_END]);
        ret = dup2(fd[WRITE_END], STDOUT_FILENO);  // standard output을 파이프에 연결
        close(fd[WRITE_END]);
        ret = execvp(new_argv[0], new_argv);
        if (ret < 0) {
            perror("Error: execvp failed");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) { /* parent process */
        close(fd[WRITE_END]);
        wait(NULL);

        FILE *fd_read = fdopen(fd[READ_END], "r");
        int i = 1;
        int argv2_len = strlen(argv[2]);

        while (fgets(read_msg, BUFFER_SIZE, fd_read) != 0) {
            int flag = 0;
            int grep_point[256] = {0};
            int grep_char[1024] = {0};
            int l = 0;
            int msg_len = strlen(read_msg);
            if (chr_flag) {
                for (int j = 0; j < msg_len; j++) {
                    for (int k = 0; k < argv2_len; k++) {
                        if (read_msg[j] == argv[2][k]) {
                            flag = 1;
                            if (l < 1024 - 1) {
                                grep_char[l++] = j;
                                grep_char[l] = -1;
                                break;
                            } else {
                                printf("!Buffer Error 1024\n");
                            }
                        }
                    }
                }
            } else if (msg_len >= argv2_len) {
                for (int j = 0; j <= (msg_len - argv2_len); j++) {
                    for (int k = 0; k < argv2_len; k++) {
                        if (read_msg[j + k] != argv[2][k]) {
                            break;
                        } else if (k == argv2_len - 1) {
                            flag = 1;
                            if (l < 256 - 1) {
                                grep_point[l++] = j;
                                grep_point[l] = -1;
                            }

                            j += argv2_len - 1;
                        }
                    }
                }
            }
            l = 0;
            if (chr_flag && flag) {
                printf("\033[0;32m[%d] \033[0m", i);  // Line number

                for (int k = 0; k < msg_len; k++) {
                    if (grep_char[l] != -1 && grep_char[l] == k) {
                        printf("\033[0;31m%c\033[0m", read_msg[k]);
                        if (l < 1024 - 1) l++;
                    } else {
                        printf("%c", read_msg[k]);
                    }
                }
            } else if (flag && !chr_flag) {
                // l = 0;
                printf("\033[0;32m[%d] \033[0m", i);

                for (int k = 0; k < msg_len; k++) {
                    if (grep_point[l] != -1 && grep_point[l] == k) {
                        printf("\033[0;31m");
                    }
                    printf("%c", read_msg[k]);
                    if (grep_point[l] != -1 && grep_point[l] + argv2_len - 1 == k) {
                        printf("\033[0m");
                        if (l < 256 - 1) l++;
                    }
                }
            }
            i++;
        }
        printf("\033[0m");

        /* close the READ end of the pipe */
        for (int i = 0; i <= count; i++) {
            free(*(new_argv + i));
        }
        free(new_argv);
        close(fd[READ_END]);
        fclose(fd_read);
    } else {
        fprintf(stderr, "Fork failed");
        return 1;
    }

    return 0;
}