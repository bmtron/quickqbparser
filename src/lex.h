#ifndef LEX_H
#define LEX_H
#include "hashtable.h"

#define START_TOKEN '<'
#define END_TOKEN '>'
#define CLOSE_TOKEN '/'
#define HEADER_TOKEN '?'
#define CLOSE_WORD "</"
#define BANK_TRAN_SECTION "BANKTRANLIST"
#define STMTTRN "STMTTRN"
#define TRNTYP "TRNTYPE"
#define DTPOSTED "DTPOSTED"
#define TRNAMT "TRNAMT"
#define REFNUM "REFNUM"
#define NAME "NAME"
#define MEMO "MEMO"
#define RESPONSIBLE_TRANSACTION_AMOUNT 200
#define TXN_FIELD_BUF_SIZE 1024
#define WORD_BUF_SIZE 512

typedef struct XmlElement {
        char* elname;
        struct XmlElement* next;
} XmlElement;
typedef struct {
        char type[TXN_FIELD_BUF_SIZE];
        char dateposted[TXN_FIELD_BUF_SIZE];
        char amount[TXN_FIELD_BUF_SIZE];
        char refnum[TXN_FIELD_BUF_SIZE];
        char name[TXN_FIELD_BUF_SIZE];
        char memo[TXN_FIELD_BUF_SIZE];
        int end_index;
} Transaction;
typedef struct {
        char* name;
        int start_idx;
} Word;
Transaction** parsedoc(int fd);
void search_for_next_char(const char buf[], int start_idx, HashTable* ht);
Word* getword(const char buf[], int start_idx, size_t file_size);
Transaction* createtransaction(const char buf[], int start_idx, size_t file_size);
int iswhitespace(const char ch);
void updateTransaction(Transaction* tr, const char* wordname, const char* data);

#endif
