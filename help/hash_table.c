#include <stdlib.h>
#include <string.h>

#include "tinycc/help/hash_table.h"

// Derived from code by Syoyo Fujita and other many contributors at https://github.com/syoyo/tinyobjloader-c

unsigned long hash_djb2(const unsigned char *str)
{
  unsigned long hash = 5381;
  int c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + (unsigned long)(c);
  }

  return hash;
}

int init_hash_table(size_t start_capacity, hash_table_t *hash_table)
{
  // TODO alloc check
  if (start_capacity < 1)
    start_capacity = HASH_TABLE_DEFAULT_SIZE;
  hash_table->hashes = (unsigned long *)malloc(start_capacity * sizeof(unsigned long));
  hash_table->entries = (hash_table_entry_t *)calloc(start_capacity, sizeof(hash_table_entry_t));
  hash_table->capacity = start_capacity;
  hash_table->n = 0;

  return 0;
}

void destroy_hash_table(hash_table_t *hash_table)
{
  free(hash_table->entries);
  free(hash_table->hashes);
}

void hash_table_clear(hash_table_t *hash_table)
{
  memset(hash_table->entries, 0, sizeof(hash_table_entry_t) * hash_table->capacity);
  hash_table->n = 0;
}

/* Insert with quadratic probing */
int hash_table_change_value(unsigned long hash, void *value, hash_table_t *hash_table)
{
  /* Insert value */
  size_t start_index = hash % hash_table->capacity;
  size_t index = start_index;
  hash_table_entry_t *start_entry = hash_table->entries + start_index;
  size_t i;
  hash_table_entry_t *entry;

  for (i = 1; hash_table->entries[index].filled; i++) {
    if (i >= hash_table->capacity)
      return HASH_TABLE_ERROR;
    index = (start_index + (i * i)) % hash_table->capacity;
  }

  entry = hash_table->entries + index;
  entry->hash = hash;
  entry->filled = 1;
  entry->value = value;

  if (index != start_index) {
    /* This is a new entry, but not the start entry, hence we need to add a next pointer to our entry */
    entry->next = start_entry->next;
    start_entry->next = entry;
  }

  return HASH_TABLE_SUCCESS;
}

int hash_table_insert(unsigned long hash, void *value, hash_table_t *hash_table)
{
  int ret = hash_table_change_value(hash, value, hash_table);
  if (ret == HASH_TABLE_SUCCESS) {
    hash_table->hashes[hash_table->n] = hash;
    hash_table->n++;
  }
  return ret;
}

int hash_table_remove(unsigned long hash, hash_table_t *hash_table)
{
  /* Get hash */
  size_t start_index = hash % hash_table->capacity;
  size_t index = start_index;
  size_t i;
  hash_table_entry_t *start_entry = hash_table->entries + index;
  hash_table_entry_t *entry = start_entry, *prev_entry = start_entry;

  while (1) {
    if (!entry->filled) {
      return HASH_TABLE_SUCCESS; // Doesn't exist in hash_table
    }

    if (entry->hash != hash) {
      prev_entry = entry;
      entry = entry->next;
      continue;
    }

    break;
  }

  if (entry == start_entry) {
    if (entry->next) {
      memcpy(start_entry, entry->next, sizeof(hash_table_entry_t));
      memset(entry->next, 0, sizeof(hash_table_entry_t));
    }
    else {
      memset(entry, 0, sizeof(hash_table_entry_t));
    }
  }
  else {
    if (entry->next) {
      prev_entry->next = entry->next;
    }
    memset(entry, 0, sizeof(hash_table_entry_t));
  }

  --hash_table->n;
  return HASH_TABLE_SUCCESS;
}

hash_table_entry_t *hash_table_find(unsigned long hash, hash_table_t *hash_table)
{
  hash_table_entry_t *entry = hash_table->entries + (hash % hash_table->capacity);
  while (entry) {
    if (entry->hash == hash && entry->filled) {
      return entry;
    }
    entry = entry->next;
  }
  return NULL;
}

void hash_table_maybe_grow(size_t new_n, hash_table_t *hash_table)
{
  size_t new_capacity;
  hash_table_t new_hash_table;
  size_t i;

  if (new_n <= hash_table->capacity) {
    return;
  }
  new_capacity = 2 * ((2 * hash_table->capacity) > new_n ? hash_table->capacity : new_n);
  /* Create a new hash table. We're not calling init_hash_table because we want to realloc the hash array */
  new_hash_table.hashes = hash_table->hashes =
      (unsigned long *)realloc((void *)hash_table->hashes, sizeof(unsigned long) * new_capacity);
  new_hash_table.entries = (hash_table_entry_t *)calloc(new_capacity, sizeof(hash_table_entry_t));
  new_hash_table.capacity = new_capacity;
  new_hash_table.n = hash_table->n;

  /* Rehash */
  for (i = 0; i < hash_table->capacity; i++) {
    hash_table_entry_t *entry = hash_table_find(hash_table->hashes[i], hash_table);
    hash_table_change_value(hash_table->hashes[i], entry->value, &new_hash_table);
  }

  free(hash_table->entries);
  (*hash_table) = new_hash_table;
}

int hash_table_exists(const char *name, hash_table_t *hash_table)
{
  unsigned long hash = hash_djb2((const unsigned char *)name);
  hash_table_entry_t *res = hash_table_find(hash, hash_table);
  return res != NULL;
}

void hash_table_set(const char *name, void *val, hash_table_t *hash_table)
{
  /* Hash name */
  unsigned long hash = hash_djb2((const unsigned char *)name);

  hash_table_entry_t *entry = hash_table_find(hash, hash_table);
  if (entry) {
    entry->value = val;
    return;
  }

  /* Expand if necessary
   * Grow until the element has been added
   */
  long ht_insert;
  size_t nex = hash_table->n + 1;
  do {
    hash_table_maybe_grow(nex, hash_table);
    ht_insert = hash_table_insert(hash, val, hash_table);
    ++nex;
  } while (ht_insert != HASH_TABLE_SUCCESS);
}

void hash_table_set_by_hash(unsigned long hash, void *val, hash_table_t *hash_table)
{
  hash_table_entry_t *entry = hash_table_find(hash, hash_table);
  if (entry) {
    entry->value = val;
    return;
  }

  /* Expand if necessary
   * Grow until the element has been added
   */
  long ht_insert;
  size_t nex = hash_table->n + 1;
  do {
    // printf("n:%li cap:%li\n", hash_table->n, hash_table->capacity);
    hash_table_maybe_grow(nex, hash_table);
    // printf("n:%li cap:%li\n", hash_table->n, hash_table->capacity);
    // usleep(10000);
    ht_insert = hash_table_insert(hash, val, hash_table);
    ++nex;
    // if (ht_insert != HASH_TABLE_SUCCESS)
    //   exit(88);
    return;
  } while (ht_insert != HASH_TABLE_SUCCESS);
}

void *hash_table_get(const char *name, hash_table_t *hash_table)
{
  unsigned long hash = hash_djb2((const unsigned char *)name);
  hash_table_entry_t *ret = hash_table_find(hash, hash_table);
  return ret ? ret->value : NULL;
}

void *hash_table_get_by_hash(unsigned long hash, hash_table_t *hash_table)
{
  hash_table_entry_t *ret = hash_table_find(hash, hash_table);
  return ret ? ret->value : NULL;
}

// /* Some 'test' code for remove */
//   // DEBUG TEST
//   unsigned long t = 482472827LU;
//   // Check
//   if (hash_table_find(t, &pjxp->collapsed_entries) != NULL)
//     puts("test-hash ERROR 1");
//   // Set
//   hash_table_insert(t, 80L, &pjxp->collapsed_entries);
//   if (pjxp->collapsed_entries.n != 1)
//     puts("test-hash ERROR 13a");
//   // Check
//   hash_table_entry_t *et = hash_table_find(t, &pjxp->collapsed_entries);
//   if (et == NULL)
//     puts("test-hash ERROR 2");
//   if (et->value != 80L)
//     puts("test-hash ERROR 3a");
//   if (et->next)
//     puts("test-hash ERROR 3b");
//   // TRY ANOTHER
//   // Check
//   if (hash_table_find(472472827LU, &pjxp->collapsed_entries) != NULL)
//     puts("test-hash ERROR 4");
//   // AND ANOTHER in the same bucket
//   hash_table_insert(t + 256LU, 40L, &pjxp->collapsed_entries);
//   if (pjxp->collapsed_entries.n != 2)
//     puts("test-hash ERROR 13b");
//   if (hash_table_find(t + 256LU, &pjxp->collapsed_entries) == NULL)
//     puts("test-hash ERROR 5");
//   if (!et->next)
//     puts("test-hash ERROR 6a");
//   if (et->next->value != 40L)
//     puts("test-hash ERROR 6b");
//   // AND ANOTHER ANOTHER in the same bucket
//   hash_table_insert(t + 512LU, 20L, &pjxp->collapsed_entries);
//   hash_table_entry_t *et2 = hash_table_find(t + 512LU, &pjxp->collapsed_entries);
//   if (pjxp->collapsed_entries.n != 3)
//     puts("test-hash ERROR 13c");
//   if (et2 == NULL)
//     puts("test-hash ERROR 7");
//   if (et2->value != 20L)
//     puts("test-hash ERROR 8a");
//   if (!et2->next)
//     puts("test-hash ERROR 8b");
//   if (et2->next->value != 40L)
//     puts("test-hash ERROR 8c");

//   // Remove THE MIDDLE ONE -- which is actually the 512 eek
//   hash_table_remove(t + 512LU, &pjxp->collapsed_entries);
//   if (hash_table_find(t + 512LU, &pjxp->collapsed_entries) != NULL)
//     puts("test-hash ERROR 9");

//   // Check again
//   et = hash_table_find(t, &pjxp->collapsed_entries);
//   if (et == NULL)
//     puts("test-hash ERROR 10z");
//   if (et->value != 80)
//     puts("test-hash ERROR 10a");
//   if (!et->next)
//     puts("test-hash ERROR 10b");
//   if (et->next->value != 40)
//     printf("test-hash ERROR 10c  '%l'\n", et->next->value);

//   et2 = hash_table_find(t + 256LU, &pjxp->collapsed_entries);
//   if (et2 == NULL)
//     puts("test-hash ERROR 11");
//   if (et2->value != 40)
//     puts("test-hash ERROR 12a");
//   if (et2->next)
//     printf("test-hash ERROR 12b  '%l'\n", et2->next->value);

//   if (pjxp->collapsed_entries.n != 2)
//     puts("test-hash ERROR 13d");
//   // FINISHED
//   puts("test-hash FINISHED");
//   // DEBUG TEST