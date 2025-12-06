#include "lex.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


Transaction** parsedoc(int fd) {
        struct stat st;
        fstat(fd, &st);
        size_t file_size = st.st_size;
        char read_buffer[file_size];
        // int read_val = read(fd, read_buffer, MAX_BUFFER_SIZE);

        int read_val;
        int base_pos_counter, search_pos_counter;
        base_pos_counter = 0;
        search_pos_counter = 0;

        Transaction** txns =
            calloc(RESPONSIBLE_TRANSACTION_AMOUNT, sizeof(Transaction*));
        int txncount = 0;

        while ((read_val = read(fd, read_buffer, file_size)) > 0) {
                printf("readval: %d\n", read_val);
                for (int i = 0; i < file_size; i++) {
                        if (read_buffer[i] == '\0') {
                                printf(
                                    "Reached null terminator at position %d\n",
                                    base_pos_counter);
                                break;
                        }
                        if (read_buffer[i] == START_TOKEN) {
                                // begin parsing
                                Word* next_word =
                                    getword(read_buffer, i + 1, file_size);
                                // search_for_next_char(read_buffer, i, ht);
                                // only concerned with stmttrn stuff
                                if (strncmp(next_word->name, STMTTRN, 7) == 0) {
                                        Transaction* new_tr = createtransaction(
                                            read_buffer, next_word->start_idx,
                                            file_size);
                                        if (txncount < 200) {
                                                txns[txncount] = new_tr;
                                                txncount++;
                                        }
                                }

                                free(next_word->name);
                                free(next_word);
                        }
                }
        }

        if (read_val < 0) {
                perror("read");
                printf("error reading from open file descriptor\n");
                return NULL;
        }

        return txns;
}

Word* getword(const char* buf, int start_idx, size_t file_size) {
        Word* word = malloc(sizeof(Word));
        word->name = malloc(sizeof(char) * WORD_BUF_SIZE);
        int letter_count = 0;
        for (int i = start_idx; i < file_size; i++) {
                if (buf[i] == CLOSE_TOKEN) {
                        i++;
                        continue;
                }
                if (buf[i] == END_TOKEN) {
                        word->name[letter_count + 1] = '\0';
                        word->start_idx = i + 1;
                        return word;
                }
                word->name[letter_count] = buf[i];
                letter_count++;
        }
        free(word->name);
        free(word);
        return NULL;
}

Transaction* createtransaction(const char* buf, int start_idx,
                               size_t file_size) {
        Transaction* tr = malloc(sizeof(Transaction));
        const int closing_token_size = 10;
        const int end_section_cmp_delim = 12;
        const int close_slice_size = 2;
        const char string_terminator = '\0';
        const char* stmttrn = "STMTTRN";

        Word* next_word = NULL;
        // find end of STMTTRN
        char closing_token[closing_token_size + 1];
        snprintf(closing_token, sizeof(closing_token), "</%s>", stmttrn);
        closing_token[closing_token_size] = string_terminator;

        char data[TXN_FIELD_BUF_SIZE];
        memset(data, '\0', TXN_FIELD_BUF_SIZE);

        int datacount = 0;
        int ending_index = start_idx;
        for (int i = start_idx; i < file_size; i++) {
                if (buf[i] == START_TOKEN) {
                        // if we reached the entire closing phrase...
                        // ( </STMTTRN> )
                        char* slice = malloc(11);
                        strncpy(slice, &buf[i], closing_token_size);
                        slice[10] = '\0';
                        if (strncmp(slice, closing_token,
                                    closing_token_size + 1) == 0) {
                                Word* temp_word = next_word;
                                if (temp_word != NULL) {
                                        free(temp_word->name);
                                        free(temp_word);
                                        next_word = NULL;
                                }
                                ending_index = i + 11;
                                free(slice);
                                datacount = 0;
                                memset(data, '\0', 1024);
                                break;
                        }
                        free(slice);
                        // if we reach the particular closing phrase
                        // for the section...
                        // e.g. ( </TRNTYPE> )
                        char* close_slice = malloc(close_slice_size + 1);
                        strncpy(close_slice, &buf[i], close_slice_size);
                        close_slice[2] = '\0';
                        if (strncmp(close_slice, (char*)CLOSE_WORD, close_slice_size) == 0) {
                                Word* ending_word =
                                    getword(buf, i + close_slice_size, file_size);
                                if (strncmp(ending_word->name, next_word->name,
                                            end_section_cmp_delim) == 0) {
                                        data[datacount] = string_terminator;
                                        datacount = 0;
                                        updateTransaction(tr, next_word->name,
                                                          data);
                                        // success, we've hit the end of the xml
                                        // section
                                        // find the next non-whitespace block
                                        // (should be '<')
                                        while (iswhitespace(buf[i])) {
                                                i++;
                                        }
                                }

                                free(ending_word->name);
                                free(ending_word);
                        }
                        free(close_slice);

                        Word* temp_word = next_word;
                        if (temp_word != NULL) {
                                free(temp_word->name);
                                free(temp_word);
                        }
                        next_word = getword(buf, i + 1, file_size);
                        i = next_word->start_idx - 1;
                        data[datacount] = string_terminator;
                        datacount = 0;
                        continue;
                }

                data[datacount] = buf[i];
                datacount++;
        }

        tr->end_index = ending_index;
        if (next_word != NULL) {
                free(next_word->name);
                free(next_word);
        }
        return tr;
}

void updateTransaction(Transaction* tr, const char* wordname,
                       const char* data) {
        if (strncmp(wordname, TRNAMT, 6) == 0) {
                strncpy(tr->amount, data, TXN_FIELD_BUF_SIZE - 2);
                tr->amount[TXN_FIELD_BUF_SIZE - 1] = '\0';
                return;
        }
        if (strncmp(wordname, TRNTYP, 6) == 0) {
                strncpy(tr->type, data, TXN_FIELD_BUF_SIZE - 2);
                tr->type[TXN_FIELD_BUF_SIZE - 1] = '\0';
                return;
        }
        if (strncmp(wordname, DTPOSTED, 8) == 0) {
                strncpy(tr->dateposted, data, TXN_FIELD_BUF_SIZE - 2);
                tr->dateposted[TXN_FIELD_BUF_SIZE - 1] = '\0';
                return;
        }
        if (strncmp(wordname, REFNUM, 6) == 0) {
                strncpy(tr->refnum, data, TXN_FIELD_BUF_SIZE - 2);
                tr->refnum[TXN_FIELD_BUF_SIZE - 1] = '\0';
                return;
        }
        if (strncmp(wordname, NAME, 4) == 0) {
                strncpy(tr->name, data, TXN_FIELD_BUF_SIZE - 2);
                tr->name[TXN_FIELD_BUF_SIZE - 1] = '\0';
                return;
        }
        if (strncmp(wordname, MEMO, 4) == 0) {
                strncpy(tr->memo, data, TXN_FIELD_BUF_SIZE - 2);
                tr->memo[TXN_FIELD_BUF_SIZE - 1] = '\0';
                return;
        }
}

int iswhitespace(const char ch) {
        if (ch == '\n' || ch == '\0' || ch == '\t' || ch == ' ') {
                return 1;
        }
        return 0;
}

