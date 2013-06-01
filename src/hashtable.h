void HtInsertValue(const char *key, void *newentry, LPCHAIN *table, unsigned int tablelen);
void HtRemoveValue(const char *key, LPCHAIN *table, unsigned int tablelen);
void *HtGetValue(const char *key, LPCHAIN *table, unsigned int tablelen);
void HtResetContents(LPCHAIN *table, unsigned int tablelen);
unsigned int __fastcall hash(unsigned char *key);

