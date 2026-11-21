/* Fixture: a // comment containing /* must not hide later declarations. */
CAPN_EXPORT void capn_free(struct capn *c); // see /* not a block comment
int missing_export_after_comment(void);
