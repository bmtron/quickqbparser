#include "parse.h"

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

        Transaction** txns =
            calloc(RESPONSIBLE_TRANSACTION_AMOUNT, sizeof(Transaction*));
        int txncount = 0;

        while ((read_val = read(fd, read_buffer, file_size)) > 0) {
                printf("readval: %d\n", read_val);
                for (size_t i = 0; i < file_size; i++) {
                        if (read_buffer[i] == '\0') {
                                printf(
                                    "Reached null terminator at position %ld\n",
                                    i);
                                break;
                        }
                        if (read_buffer[i] == START_TOKEN) {
                                // begin parsing
                                Word* next_word =
                                    getword(read_buffer, i + 1, file_size);
                                // search_for_next_char(read_buffer, i, ht);
                                // only concerned with stmttrn stuff
                                if (strncmp(next_word->name, STMTTRN,
                                            strlen(STMTTRN)) == 0) {
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
        for (size_t i = start_idx; i < file_size; i++) {
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
        for (size_t i = start_idx; i < file_size; i++) {
                if (buf[i] == START_TOKEN) {
                        // if we reached the entire closing phrase...
                        // ( </STMTTRN> )
                        if (checkForStmtEnd(buf, &next_word, data,
                                            &ending_index, &datacount, i,
                                            closing_token,
                                            closing_token_size) == 1) {
                                break;
                        }
                        // if we reach the particular closing phrase
                        // for the section...
                        // e.g. ( </TRNTYPE> )
                        checkForSectionEnd(buf, file_size, close_slice_size,
                                           data, datacount, next_word, &i, tr);

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
int checkForStmtEnd(const char* buf, Word** next_word, char* data,
                    int* ending_index, int* datacount, size_t buf_idx,
                    char* closing_token, int closing_token_size) {
        char slice[closing_token_size + 1];
        strncpy(slice, &buf[buf_idx], closing_token_size);
        slice[closing_token_size] = '\0';
        if (strncmp(slice, closing_token, closing_token_size + 1) == 0) {
                Word* temp_word = *next_word;
                if (temp_word != NULL) {
                        free(temp_word->name);
                        free(temp_word);
                        *next_word = NULL;
                }
                *ending_index = buf_idx + closing_token_size + 1;
                *datacount = 0;
                memset(data, '\0', TXN_FIELD_BUF_SIZE);
                return 1;
        }
        return 0;
}

void checkForSectionEnd(const char* buf, size_t file_size, int close_slice_size,
                        char* data, int datacount, Word* next_word,
                        size_t* buf_idx, Transaction* tr) {
        const int end_section_cmp_delim = 12;
        const char string_terminator = '\0';
        char close_slice[close_slice_size + 1];
        strncpy(close_slice, &buf[*buf_idx], close_slice_size);
        close_slice[close_slice_size] = '\0';
        if (strncmp(close_slice, (char*)CLOSE_WORD, close_slice_size) == 0) {
                Word* ending_word =
                    getword(buf, *buf_idx + close_slice_size, file_size);
                if (strncmp(ending_word->name, next_word->name,
                            end_section_cmp_delim) == 0) {
                        data[datacount] = string_terminator;
                        datacount = 0;
                        updateTransaction(tr, next_word->name, data);
                        // success, we've hit the end of the xml
                        // section
                        // find the next non-whitespace block
                        // (should be '<')
                        while (iswhitespace(buf[*buf_idx])) {
                                *buf_idx = *buf_idx + 1;
                        }
                }

                free(ending_word->name);
                free(ending_word);
        }
}
void updateTransaction(Transaction* tr, const char* wordname,
                       const char* data) {
        const int max_buf_size_without_null_term = TXN_FIELD_BUF_SIZE - 2;
        const int max_buf_size_with_null_term = TXN_FIELD_BUF_SIZE - 1;
        if (strncmp(wordname, TRNAMT, 6) == 0) {
                strncpy(tr->amount, data, max_buf_size_without_null_term);
                tr->amount[max_buf_size_with_null_term] = '\0';
                return;
        }
        if (strncmp(wordname, TRNTYP, 6) == 0) {
                strncpy(tr->type, data, max_buf_size_without_null_term);
                tr->type[max_buf_size_with_null_term] = '\0';
                return;
        }
        if (strncmp(wordname, DTPOSTED, 8) == 0) {
                strncpy(tr->dateposted, data, max_buf_size_without_null_term);
                tr->dateposted[max_buf_size_with_null_term] = '\0';
                return;
        }
        if (strncmp(wordname, REFNUM, 6) == 0) {
                strncpy(tr->refnum, data, max_buf_size_without_null_term);
                tr->refnum[max_buf_size_with_null_term] = '\0';
                return;
        }
        if (strncmp(wordname, NAME, 4) == 0) {
                strncpy(tr->name, data, max_buf_size_without_null_term);
                tr->name[max_buf_size_with_null_term] = '\0';
                return;
        }
        if (strncmp(wordname, MEMO, 4) == 0) {
                strncpy(tr->memo, data, max_buf_size_without_null_term);
                tr->memo[max_buf_size_with_null_term] = '\0';
                return;
        }
}

int iswhitespace(const char ch) {
        if (ch == '\n' || ch == '\0' || ch == '\t' || ch == ' ') {
                return 1;
        }
        return 0;
}
