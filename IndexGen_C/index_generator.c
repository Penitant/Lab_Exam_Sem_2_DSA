#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define INITIAL_TABLE_SIZE 1000
#define LOAD_FACTOR_THRESHOLD 0.7
#define MAX_WORD_LENGTH 256

typedef struct LineNode {
    int line_number;
    struct LineNode* next;
} LineNode;

typedef struct HashEntry {
    char* word;
    LineNode* line_list;
    int probe_distance;
    int occupied;
} HashEntry;

typedef struct HashTable {
    HashEntry* entries;
    int size;
    int count;
} HashTable;

unsigned long hash_function(const char* str, int table_size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % table_size;
}

HashTable* create_hash_table(int size) {
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));
    table->size = size;
    table->count = 0;
    table->entries = (HashEntry*)calloc(size, sizeof(HashEntry));
    for (int i = 0; i < size; i++) {
        table->entries[i].word = NULL;
        table->entries[i].line_list = NULL;
        table->entries[i].probe_distance = 0;
        table->entries[i].occupied = 0;
    }
    return table;
}

LineNode* create_line_node(int line_number) {
    LineNode* node = (LineNode*)malloc(sizeof(LineNode));
    node->line_number = line_number;
    node->next = NULL;
    return node;
}

void add_line_number(LineNode** head, int line_number) {
    LineNode* current = *head;
    LineNode* prev = NULL;
    
    while (current != NULL && current->line_number < line_number) {
        prev = current;
        current = current->next;
    }
    
    if (current != NULL && current->line_number == line_number) {
        return;
    }
    
    LineNode* new_node = create_line_node(line_number);
    
    if (prev == NULL) {
        new_node->next = *head;
        *head = new_node;
    } else {
        new_node->next = current;
        prev->next = new_node;
    }
}

void robin_hood_insert(HashTable* table, char* word, int line_number);

HashTable* resize_hash_table(HashTable* old_table) {
    int new_size = old_table->size * 2;
    HashTable* new_table = create_hash_table(new_size);
    
    for (int i = 0; i < old_table->size; i++) {
        if (old_table->entries[i].occupied) {
            LineNode* current = old_table->entries[i].line_list;
            while (current != NULL) {
                robin_hood_insert(new_table, old_table->entries[i].word, current->line_number);
                current = current->next;
            }
        }
    }
    
    return new_table;
}

void robin_hood_insert(HashTable* table, char* word, int line_number) {
    if ((double)table->count / table->size > LOAD_FACTOR_THRESHOLD) {
        HashTable* new_table = resize_hash_table(table);
        table->entries = new_table->entries;
        table->size = new_table->size;
        table->count = new_table->count;
        free(new_table);
    }
    
    unsigned long hash = hash_function(word, table->size);
    int index = hash;
    int distance = 0;
    
    char* insert_word = strdup(word);
    LineNode* insert_list = create_line_node(line_number);
    int insert_distance = 0;
    
    while (1) {
        if (!table->entries[index].occupied) {
            table->entries[index].word = insert_word;
            table->entries[index].line_list = insert_list;
            table->entries[index].probe_distance = insert_distance;
            table->entries[index].occupied = 1;
            table->count++;
            return;
        }
        
        if (strcmp(table->entries[index].word, insert_word) == 0) {
            add_line_number(&table->entries[index].line_list, line_number);
            free(insert_word);
            free(insert_list);
            return;
        }
        
        if (table->entries[index].probe_distance < insert_distance) {
            char* temp_word = table->entries[index].word;
            LineNode* temp_list = table->entries[index].line_list;
            int temp_distance = table->entries[index].probe_distance;
            
            table->entries[index].word = insert_word;
            table->entries[index].line_list = insert_list;
            table->entries[index].probe_distance = insert_distance;
            
            insert_word = temp_word;
            insert_list = temp_list;
            insert_distance = temp_distance;
        }
        
        index = (index + 1) % table->size;
        insert_distance++;
    }
}

int is_separator(char c) {
    return c == ' ' || c == '\n' || c == '.' || c == ',' || c == ':' || c == ';' || c == '\t' || c == '\r';
}

void to_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

void process_file(const char* input_file, HashTable* table) {
    FILE* file = fopen(input_file, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open input file %s\n", input_file);
        exit(1);
    }
    
    char word[MAX_WORD_LENGTH];
    int word_index = 0;
    int line_number = 1;
    int c;
    
    while ((c = fgetc(file)) != EOF) {
        if (is_separator(c)) {
            if (word_index > 0) {
                word[word_index] = '\0';
                to_lowercase(word);
                robin_hood_insert(table, word, line_number);
                word_index = 0;
            }
            if (c == '\n') {
                line_number++;
            }
        } else {
            if (word_index < MAX_WORD_LENGTH - 1) {
                word[word_index++] = c;
            }
        }
    }
    
    if (word_index > 0) {
        word[word_index] = '\0';
        to_lowercase(word);
        robin_hood_insert(table, word, line_number);
    }
    
    fclose(file);
}

typedef struct WordEntry {
    char* word;
    LineNode* line_list;
} WordEntry;

int compare_words(const void* a, const void* b) {
    WordEntry* entry_a = (WordEntry*)a;
    WordEntry* entry_b = (WordEntry*)b;
    return strcmp(entry_a->word, entry_b->word);
}

void generate_index_file(HashTable* table, const char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot open output file %s\n", output_file);
        exit(1);
    }
    
    WordEntry* sorted_entries = (WordEntry*)malloc(table->count * sizeof(WordEntry));
    int entry_count = 0;
    
    for (int i = 0; i < table->size; i++) {
        if (table->entries[i].occupied) {
            sorted_entries[entry_count].word = table->entries[i].word;
            sorted_entries[entry_count].line_list = table->entries[i].line_list;
            entry_count++;
        }
    }
    
    qsort(sorted_entries, entry_count, sizeof(WordEntry), compare_words);
    
    fprintf(file, "WORD INDEX (Lexicographic Order)\n");
    fprintf(file, "=================================\n\n");
    
    for (int i = 0; i < entry_count; i++) {
        fprintf(file, "%s: ", sorted_entries[i].word);
        LineNode* current = sorted_entries[i].line_list;
        while (current != NULL) {
            fprintf(file, "%d", current->line_number);
            if (current->next != NULL) {
                fprintf(file, ", ");
            }
            current = current->next;
        }
        fprintf(file, "\n");
    }
    
    free(sorted_entries);
    fclose(file);
}

void free_hash_table(HashTable* table) {
    for (int i = 0; i < table->size; i++) {
        if (table->entries[i].occupied) {
            free(table->entries[i].word);
            LineNode* current = table->entries[i].line_list;
            while (current != NULL) {
                LineNode* temp = current;
                current = current->next;
                free(temp);
            }
        }
    }
    free(table->entries);
    free(table);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }
    
    char* input_file = argv[1];
    char* output_file = argv[2];
    
    clock_t start_time = clock();
    
    HashTable* table = create_hash_table(INITIAL_TABLE_SIZE);
    
    process_file(input_file, table);
    
    generate_index_file(table, output_file);
    
    clock_t end_time = clock();
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("Index generated: %s\n", output_file);
    printf("Words: %d | Time: %.6f sec\n", table->count, time_taken);
    
    free_hash_table(table);
    
    return 0;
}
