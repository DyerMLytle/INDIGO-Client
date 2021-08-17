typedef struct Dictionary t Dictionary;

Dictionary *create table(void);
void destroy table(Dictionary *);

int add word(Dictionary *, const char *key, const char *defn);
char *find word(const Dictionary *, const char *key);
void delete word(Dictionary *, const char *key);

define HASHSIZE 101

struct Nlist {
  char *word; /* search word */
  char *defn; /* word definition */
  struct Nlist *next; /* pointer to next entry in chain */
};

struct Dictionary t {
  struct Nlist *table[HASHSIZE]; /* table is an array of pointers to entries */
};

static unsigned hash function(const char *str) {
  /* Hashing function converts a string to an index within hash table. */

  const int HashValue = 31;
  unsigned h;

  for (h = 0; *str != ’\0’; ++str)
    h = *str + HashValue * h;
  return h % HASHSIZE;
}

int add word(Dictionary *dict, const char *key, const char *defn) {
  /* Add new word to table. Replaces old definition if word already exists.
   * Return 0 if successful, and -1 is fails. */

  unsigned i = hash function(key); /* get table index */
  struct Nlist *pnode = dict−>table[i];

  while (pnode && strcmp(pnode−>word, key) != 0) /* search chain */
    pnode = pnode−>next;

  if (pnode) { /* match found, replace definition */
    char *str = allocate string(defn);
    if (str == NULL) /* allocation fails, return fail and keep old defn */
      return −1;

    free(pnode−>defn);
    pnode−>defn = str;

  } else { /* no match, add new entry to head of chain */
    pnode = makenode(key, defn);
    if (pnode == NULL)
      return −1;
      
    pnode−>next = dict−>table[i];
    dict−>table[i] = pnode;
  }
  return 0;
}

char *find word(const Dictionary *dict, const char *key) {
  /* Find definition for keyword. Return NULL if key not found. */

  unsigned i = hash function(key); /* get table index */
  struct Nlist *pnode = dict−>table[i];

  while (pnode && strcmp(pnode−>word, key) != 0) /* search index chain */
    pnode = pnode−>next;

  if (pnode) /* match found */
    return pnode−>defn;
  return NULL;
}
