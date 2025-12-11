#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "parse.h"

int main(int argc, char** argv) {
        printf("PARSINATOR\n");
        int fd = open("/home/bmtron/projects/c-misc/xmlparse/activity.qbo",
                      O_RDWR, S_IRWXU);
        printf("fd: %d\n", fd);
        if (fd < 0) {
                perror("open");
        }
        Transaction** result = parsedoc(fd);
        close(fd);
        int write_fd = open("/home/bmtron/projects/c-misc/xmlparse/result.md",
                            O_CREAT | O_RDWR, S_IRWXU);
        const int REFNUM_LEN = 18;
        const int ENTRY_SIZE = REFNUM_LEN + 1;  // +1 for newline

        int txncount = 0;
        for (int i = 0; i < 200; i++) {
            if (result[i] != NULL) {
                ++txncount;
            }

        }
        char* buf =
            calloc(txncount * 19, 1);  // Only allocate for actual transactions

        for (int i = 0; i < txncount; i++) {  // Loop to txncount, not 200!
                int offset = i * 19;
                if (result[i] != NULL) {  // Safety check
                        strncpy(&buf[offset], result[i]->refnum, 18);
                        buf[offset + 18] = '\n';
                        free(result[i]);
                }
        }
        free(result);
        write(write_fd, buf, txncount * 19);
        close(write_fd);
        free(buf);

        return 0;
}
