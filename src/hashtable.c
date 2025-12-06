#include "hashtable.h"

unsigned int hash_int(const void* key) {
        int key_value = *(int*)key;
        unsigned int hash_value;
        hash_value = key_value * HASH_CONSTANT + key_value;
        return hash_value;
}
unsigned int hash_string(const void* key) {
        char* key_value = (char*)key;
        char* first_char = &key_value[0];
        unsigned int hash_value;
        hash_value = 1;

        while (*first_char != '\0') {
                hash_value =
                    hash_value +
                    (unsigned int)(*first_char * HASH_CONSTANT + *first_char);
                ++first_char;
        }
        return hash_value;
}
unsigned int hash_ptr(const void* key) {
        unsigned int hash_value;
        uintptr_t ptr_address = (uintptr_t)key;
        hash_value = ptr_address * HASH_CONSTANT + ptr_address;
        return hash_value;
}
HashTable* hash_table_create(int key_type, int value_type) {
        HashTable* hash_table = malloc(sizeof(HashTable));

        hash_table->hash_key_type = key_type;
        hash_table->size = TABLE_SIZE;
        hash_table->bucket_count = 0;

        switch (key_type) {
                case HASH_TYPE_INT:
                        hash_table->hash_func = hash_int;
                        break;
                case HASH_TYPE_STRING:
                        hash_table->hash_func = hash_string;
                        break;
                case HASH_TYPE_PTR:
                        hash_table->hash_func = hash_ptr;
                        break;
                default:
                        return NULL;
        }

        switch (value_type) {
                case HASH_TYPE_INT:
                case HASH_TYPE_PTR:
                case HASH_TYPE_STRING:
                        hash_table->hash_value_type = value_type;
                        break;
                default:
                        return NULL;
        }

        hash_table->buckets = calloc(hash_table->size, sizeof(HashEntry*));

        return hash_table;
}
int hash_table_put(HashTable* hash_table, void* key, void* value) {
        unsigned int hash_value = hash_table->hash_func(key);
        unsigned int bucket_idx = hash_value % hash_table->size;

        HashEntry* existing_hash_entry = hash_table->buckets[bucket_idx];

        while (existing_hash_entry != NULL) {
                if (key_compare(existing_hash_entry->key, key,
                                hash_table->hash_key_type) == 1) {
                        value_destructor(existing_hash_entry->value);
                        existing_hash_entry->value = value;
                        return 1;
                }

                existing_hash_entry = existing_hash_entry->next;
        }

        HashEntry* hash_entry = malloc(sizeof(HashEntry));
        hash_entry->key = key;
        hash_entry->value = value;
        hash_entry->next = hash_table->buckets[bucket_idx];
        hash_table->buckets[bucket_idx] = hash_entry;

        return 1;
}

void* hash_table_get(HashTable* hash_table, void* key) {
        unsigned int hash_value = hash_table->hash_func(key);
        HashEntry* target_entry;
        unsigned int bucket_idx = hash_value % hash_table->size;

        HashEntry* existing_hash_entry = hash_table->buckets[bucket_idx];

        if (existing_hash_entry == NULL) {
                return NULL;
        }
        if (existing_hash_entry->next == NULL &&
            key_compare(existing_hash_entry->key, key,
                        hash_table->hash_key_type) == 1) {
                return existing_hash_entry->value;
        }

        HashEntry* first = existing_hash_entry;

        while (existing_hash_entry != NULL) {
                if (key_compare(existing_hash_entry->key, key,
                                hash_table->hash_key_type) == 1) {
                        return existing_hash_entry->value;
                }
                // deal with ptr type later...
                //
                // if (hash_table->hash_key_type == HASH_TYPE_PTR)
                existing_hash_entry = existing_hash_entry->next;
        }
        return NULL;
}

int key_compare(const void* key1, const void* key2, int key_type) {
        int key_match;
        key_match = 0;

        // compare the keys
        if (key_type == HASH_TYPE_INT) {
                if (*(int*)key1 == *(int*)key2) {
                        key_match = 1;
                }
        }
        if (key_type == HASH_TYPE_STRING) {
                if (strcmp((char*)key1, (char*)key2) == 0) {
                        key_match = 1;
                }
        }

        return key_match;
}

void key_destructor(void* key) {
        if (key == NULL) {
                return;
        }
        free(key);
}
void value_destructor(void* value) {
        if (value == NULL) {
                return;
        }
        free(value);
}
int hash_table_remove(HashTable* hash_table, void* key) {
        unsigned int hash_value = hash_table->hash_func(key);
        unsigned int hash_idx = hash_value % hash_table->size;
        HashEntry* target = hash_table->buckets[hash_idx];

        if (target == NULL) {
                return 0;
        }

        if (key_compare(target->key, key, hash_table->hash_key_type) == 1) {
                hash_table->buckets[hash_idx] = target->next;
                key_destructor(target->key);
                value_destructor(target->value);
                free(target);
                return 1;
        }

        HashEntry* first = target;
        HashEntry* previous = target;
        while (target != NULL) {
                if (key_compare(key, target->key, hash_table->hash_key_type) ==
                    1) {
                        HashEntry* next_node = target->next;
                        key_destructor(target->key);
                        value_destructor(target->value);
                        free(target);
                        previous->next = next_node;
                        return 1;
                }
                previous = target;
                target = target->next;
        }

        return -1;
}
void print(HashTable* hash_table) {
        int count;
        count = 0;

        while (count < hash_table->size) {
                HashEntry* entry = hash_table->buckets[count];
                HashEntry* first = entry;
                while (entry != NULL) {
                        switch (hash_table->hash_key_type) {
                                case HASH_TYPE_INT:
                                        printf("Key: %d ", *(int*)entry->key);
                                        break;
                                case HASH_TYPE_STRING:
                                        printf("Key: %s ", (char*)entry->key);
                                        break;
                                case HASH_TYPE_PTR:
                                        printf("Key: %p ", entry->key);
                                        break;
                                default:
                                        break;
                        }
                        switch (hash_table->hash_value_type) {
                                case HASH_TYPE_INT:
                                        printf("Value: %d\n",
                                               *(int*)entry->value);
                                        break;
                                case HASH_TYPE_STRING:
                                        printf("Value: %s\n",
                                               (char*)entry->value);
                                        break;
                                case HASH_TYPE_PTR:
                                        printf("Value: %p\n", entry->value);
                                        break;
                                default:
                                        break;
                        }
                        entry = entry->next;
                }
                ++count;
        }
}

void hash_table_destroy(HashTable* hash_table) {
        if (hash_table == NULL) {
                return;
        }
        for (int i = 0; i < hash_table->size; i++) {
                HashEntry* entry = hash_table->buckets[i];
                while (entry != NULL) {
                        HashEntry* temp_entry = entry;
                        key_destructor(entry->key);
                        value_destructor(entry->value);
                        entry = temp_entry->next;
                        free(temp_entry);
                }
        }

        free(hash_table->buckets);
        free(hash_table);
}
