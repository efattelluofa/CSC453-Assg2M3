symptr sym_insert(char *s, int term);
void set_startsym(char *s);
symptr p_lookup(pptr set, symptr sptr);
pptr p_add(pptr set, symptr sptr);
plist MkProdList(plist pl, pptr bod);
pptr ProdAttach(pptr pbody, symptr sptr);
pptr ProdCreat(symptr sptr);
symptr sym_add(char *s, int term);

int is_nullable(pptr p0);
void CompFirst();
pptr GetFirst(pptr L);

void CompFollow();

void PrintSets();
