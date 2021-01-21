#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>

// Derived from code by Syoyo Fujita and other many contributors at https://github.com/syoyo/tinyobjloader-c

#define HASH_TABLE_ERROR 1
#define HASH_TABLE_SUCCESS 0
#define HASH_TABLE_DEFAULT_SIZE 10

typedef struct hash_table_entry_t {
  unsigned long hash;
  int filled;
  int pad0;
  void *value;

  struct hash_table_entry_t *next;
} hash_table_entry_t;

typedef struct hash_table_t {
  unsigned long *hashes;
  hash_table_entry_t *entries;
  size_t capacity;
  size_t n;
} hash_table_t;

// extern "C" {
unsigned long hash_djb2(const unsigned char *str);
int init_hash_table(size_t start_capacity, hash_table_t *hash_table);
void destroy_hash_table(hash_table_t *hash_table);
void hash_table_clear(hash_table_t *hash_table);
int hash_table_change_value(unsigned long hash, void *value, hash_table_t *hash_table);
int hash_table_insert(unsigned long hash, void *value, hash_table_t *hash_table);
int hash_table_remove(unsigned long hash, hash_table_t *hash_table);
hash_table_entry_t *hash_table_find(unsigned long hash, hash_table_t *hash_table);
void hash_table_maybe_grow(size_t new_n, hash_table_t *hash_table);
int hash_table_exists(const char *name, hash_table_t *hash_table);
void hash_table_set(const char *name, void *val, hash_table_t *hash_table);
void hash_table_set_by_hash(unsigned long hash, void *val, hash_table_t *hash_table);
void *hash_table_get(const char *name, hash_table_t *hash_table);
void *hash_table_get_by_hash(unsigned long hash, hash_table_t *hash_table);
// }
#endif // HASH_TABLE_H
