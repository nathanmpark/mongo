/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

/*
 * Initialize a static WT_CURSOR structure.
 */
#define	WT_CURSOR_STATIC_INIT(n,					\
	get_key,							\
	get_value,							\
	set_key,							\
	set_value,							\
	compare,							\
	equals,								\
	next,								\
	prev,								\
	reset,								\
	search,								\
	search_near,							\
	insert,								\
	update,								\
	remove,								\
	reconfigure,							\
	close)								\
	static const WT_CURSOR n = {					\
	NULL,				/* session */			\
	NULL,				/* uri */			\
	NULL,				/* key_format */		\
	NULL,				/* value_format */		\
	(int (*)(WT_CURSOR *, ...))(get_key),				\
	(int (*)(WT_CURSOR *, ...))(get_value),				\
	(void (*)(WT_CURSOR *, ...))(set_key),				\
	(void (*)(WT_CURSOR *, ...))(set_value),			\
	(int (*)(WT_CURSOR *, WT_CURSOR *, int *))(compare),		\
	(int (*)(WT_CURSOR *, WT_CURSOR *, int *))(equals),		\
	next,								\
	prev,								\
	reset,								\
	search,								\
	(int (*)(WT_CURSOR *, int *))(search_near),			\
	insert,								\
	update,								\
	remove,								\
	close,								\
	(int (*)(WT_CURSOR *, const char *))(reconfigure),		\
	{ NULL, NULL },			/* TAILQ_ENTRY q */		\
	0,				/* recno key */			\
	{ 0 },				/* recno raw buffer */		\
	NULL,				/* json_private */		\
	NULL,				/* lang_private */		\
	{ NULL, 0, 0, NULL, 0 },	/* WT_ITEM key */		\
	{ NULL, 0, 0, NULL, 0 },	/* WT_ITEM value */		\
	0,				/* int saved_err */		\
	NULL,				/* internal_uri */		\
	0				/* uint32_t flags */		\
}

struct __wt_cursor_backup_entry {
	char *name;			/* File name */
	WT_DATA_HANDLE *handle;		/* Handle */
};
struct __wt_cursor_backup {
	WT_CURSOR iface;

	size_t next;			/* Cursor position */
	FILE *bfp;			/* Backup file */
	uint32_t maxid;			/* Maximum log file ID seen */

	WT_CURSOR_BACKUP_ENTRY *list;	/* List of files to be copied. */
	size_t list_allocated;
	size_t list_next;
};
#define	WT_CURSOR_BACKUP_ID(cursor)	(((WT_CURSOR_BACKUP *)cursor)->maxid)

struct __wt_cursor_btree {
	WT_CURSOR iface;

	WT_BTREE *btree;		/* Enclosing btree */

	/*
	 * The following fields are set by the search functions as a precursor
	 * to page modification: we have a page, a WT_COL/WT_ROW slot on the
	 * page, an insert head, insert list and a skiplist stack (the stack of
	 * skiplist entries leading to the insert point).  The search functions
	 * also return the relationship of the search key to the found key.
	 */
	WT_REF	  *ref;			/* Current page */
	uint32_t   slot;		/* WT_COL/WT_ROW 0-based slot */

	WT_INSERT_HEAD	*ins_head;	/* Insert chain head */
	WT_INSERT	*ins;		/* Current insert node */
					/* Search stack */
	WT_INSERT	**ins_stack[WT_SKIP_MAXDEPTH];

					/* Next item(s) found during search */
	WT_INSERT	*next_stack[WT_SKIP_MAXDEPTH];

	uint32_t page_deleted_count;	/* Deleted items on the page */

	uint64_t recno;			/* Record number */

	/*
	 * The search function sets compare to:
	 *	< 1 if the found key is less than the specified key
	 *	  0 if the found key matches the specified key
	 *	> 1 if the found key is larger than the specified key
	 */
	int	compare;

	/*
	 * A key returned from a binary search or cursor movement on a row-store
	 * page; if we find an exact match on a row-store leaf page in a search
	 * operation, keep a copy of key we built during the search to avoid
	 * doing the additional work of getting the key again for return to the
	 * application. Note, this only applies to exact matches when searching
	 * disk-image structures, so it's not, for example, a key from an insert
	 * list. Additionally, this structure is used to build keys when moving
	 * a cursor through a row-store leaf page.
	 */
	WT_ITEM *row_key, _row_key;

	/*
	 * It's relatively expensive to calculate the last record on a variable-
	 * length column-store page because of the repeat values.  Calculate it
	 * once per page and cache it.  This value doesn't include the skiplist
	 * of appended entries on the last page.
	 */
	uint64_t last_standard_recno;

	/*
	 * For row-store pages, we need a single item that tells us the part of
	 * the page we're walking (otherwise switching from next to prev and
	 * vice-versa is just too complicated), so we map the WT_ROW and
	 * WT_INSERT_HEAD insert array slots into a single name space: slot 1
	 * is the "smallest key insert list", slot 2 is WT_ROW[0], slot 3 is
	 * WT_INSERT_HEAD[0], and so on.  This means WT_INSERT lists are
	 * odd-numbered slots, and WT_ROW array slots are even-numbered slots.
	 */
	uint32_t row_iteration_slot;	/* Row-store iteration slot */

	/*
	 * Variable-length column-store values are run-length encoded and may
	 * be overflow values or Huffman encoded. To avoid repeatedly reading
	 * overflow values or decompressing encoded values, process it once and
	 * store the result in a temporary buffer.  The cip_saved field is used
	 * to determine if we've switched columns since our last cursor call.
	 */
	WT_COL *cip_saved;		/* Last iteration reference */

	/*
	 * We don't instantiate prefix-compressed keys on pages where there's no
	 * Huffman encoding because we don't want to waste memory if only moving
	 * a cursor through the page, and it's faster to build keys while moving
	 * through the page than to roll-forward from a previously instantiated
	 * key (we don't instantiate all of the keys, just the ones at binary
	 * search points).  We can't use the application's WT_CURSOR key field
	 * as a copy of the last-returned key because it may have been altered
	 * by the API layer, for example, dump cursors.  Instead we store the
	 * last-returned key in a temporary buffer.  The rip_saved field is used
	 * to determine if the key in the temporary buffer has the prefix needed
	 * for building the current key.
	 */
	WT_ROW *rip_saved;		/* Last-returned key reference */

	/*
	 * A temporary buffer for caching RLE values for column-store files (if
	 * RLE is non-zero, then we don't unpack the value every time we move
	 * to the next cursor position, we re-use the unpacked value we stored
	 * here the first time we hit the value).
	 *
	 * A temporary buffer for building on-page keys when searching row-store
	 * files.
	 */
	WT_ITEM *tmp, _tmp;

	/*
	 * The update structure allocated by the row- and column-store modify
	 * functions, used to avoid a data copy in the WT_CURSOR.update call.
	 */
	WT_UPDATE *modify_update;

	/*
	 * Fixed-length column-store items are a single byte, and it's simpler
	 * and cheaper to allocate the space for it now than keep checking to
	 * see if we need to grow the buffer.
	 */
	uint8_t v;			/* Fixed-length return value */

	uint8_t	append_tree;		/* Cursor appended to the tree */

#define	WT_CBT_ACTIVE		0x01	/* Active in the tree */
#define	WT_CBT_ITERATE_APPEND	0x02	/* Col-store: iterating append list */
#define	WT_CBT_ITERATE_NEXT	0x04	/* Next iteration configuration */
#define	WT_CBT_ITERATE_PREV	0x08	/* Prev iteration configuration */
#define	WT_CBT_MAX_RECORD	0x10	/* Col-store: past end-of-table */
#define	WT_CBT_SEARCH_SMALLEST	0x20	/* Row-store: small-key insert list */
	uint8_t flags;
};

struct __wt_cursor_bulk {
	WT_CURSOR_BTREE cbt;

	WT_REF	*ref;			/* The leaf page */
	WT_PAGE *leaf;

	/*
	 * Variable-length column store compares values during bulk load as
	 * part of RLE compression, row-store compares keys during bulk load
	 * to avoid corruption.
	 */
	WT_ITEM last;			/* Last key/value seen */

	/*
	 * Variable-length column-store RLE counter (also overloaded to mean
	 * the first time through the bulk-load insert routine, when set to 0).
	 */
	uint64_t rle;

	/*
	 * Fixed-length column-store current entry in memory chunk count, and
	 * the maximum number of records per chunk.
	 */
	uint32_t entry;			/* Entry count */
	uint32_t nrecs;			/* Max records per chunk */

	/* Special bitmap bulk load for fixed-length column stores. */
	bool	bitmap;

	void	*reconcile;		/* Reconciliation information */
};

struct __wt_cursor_config {
	WT_CURSOR iface;
};

struct __wt_cursor_data_source {
	WT_CURSOR iface;

	WT_COLLATOR *collator;		/* Configured collator */
	int collator_owned;		/* Collator needs to be terminated */

	WT_CURSOR *source;		/* Application-owned cursor */
};

struct __wt_cursor_dump {
	WT_CURSOR iface;

	WT_CURSOR *child;
};

struct __wt_cursor_index {
	WT_CURSOR iface;

	WT_TABLE *table;
	WT_INDEX *index;
	const char *key_plan, *value_plan;

	WT_CURSOR *child;
	WT_CURSOR **cg_cursors;
	uint8_t	*cg_needvalue;
};

struct __wt_cursor_join_iter {
	WT_SESSION_IMPL		*session;
	WT_CURSOR_JOIN		*cjoin;
	WT_CURSOR_JOIN_ENTRY	*entry;
	WT_CURSOR		*cursor;
	WT_ITEM			*curkey;
	bool			 advance;
};

struct __wt_cursor_join_endpoint {
	WT_ITEM			 key;
	uint8_t			 recno_buf[10];	/* holds packed recno */
	WT_CURSOR		*cursor;

#define	WT_CURJOIN_END_LT	0x01		/* include values <  cursor */
#define	WT_CURJOIN_END_EQ	0x02		/* include values == cursor */
#define	WT_CURJOIN_END_GT	0x04		/* include values >  cursor */
#define	WT_CURJOIN_END_GE	(WT_CURJOIN_END_GT | WT_CURJOIN_END_EQ)
#define	WT_CURJOIN_END_LE	(WT_CURJOIN_END_LT | WT_CURJOIN_END_EQ)
#define	WT_CURJOIN_END_OWN_KEY	0x08		/* must free key's data */
	uint8_t			 flags;		/* range for this endpoint */
};

struct __wt_cursor_join_entry {
	WT_INDEX		*index;
	WT_CURSOR		*main;		/* raw main table cursor */
	WT_BLOOM		*bloom;		/* Bloom filter handle */
	uint32_t		 bloom_bit_count; /* bits per item in bloom */
	uint32_t		 bloom_hash_count; /* hash functions in bloom */
	uint64_t		 count;		/* approx number of matches */

#define	WT_CURJOIN_ENTRY_BLOOM		0x01	/* use a bloom filter */
#define	WT_CURJOIN_ENTRY_DISJUNCTION	0x02	/* endpoints are or-ed */
#define	WT_CURJOIN_ENTRY_OWN_BLOOM	0x04	/* this entry owns the bloom */
	uint8_t			 flags;

	WT_CURSOR_JOIN_ENDPOINT	*ends;		/* reference endpoints */
	size_t			 ends_allocated;
	size_t			 ends_next;

	WT_JOIN_STATS		 stats;		/* Join statistics */
};

struct __wt_cursor_join {
	WT_CURSOR iface;

	WT_TABLE		*table;
	const char		*projection;
	WT_CURSOR_JOIN_ITER	*iter;
	WT_CURSOR_JOIN_ENTRY	*entries;
	size_t			 entries_allocated;
	u_int			 entries_next;
	uint8_t			 recno_buf[10];	/* holds packed recno */

#define	WT_CURJOIN_ERROR		0x01	/* Error in initialization */
#define	WT_CURJOIN_INITIALIZED		0x02	/* Successful initialization */
#define	WT_CURJOIN_SKIP_FIRST_LEFT	0x04	/* First check not needed */
	uint8_t			 flags;
};

struct __wt_cursor_json {
	char	*key_buf;		/* JSON formatted string */
	char	*value_buf;		/* JSON formatted string */
	WT_CONFIG_ITEM key_names;	/* Names of key columns */
	WT_CONFIG_ITEM value_names;	/* Names of value columns */
};

struct __wt_cursor_log {
	WT_CURSOR iface;

	WT_LSN		*cur_lsn;	/* LSN of current record */
	WT_LSN		*next_lsn;	/* LSN of next record */
	WT_ITEM		*logrec;	/* Copy of record for cursor */
	WT_ITEM		*opkey, *opvalue;	/* Op key/value copy */
	const uint8_t	*stepp, *stepp_end;	/* Pointer within record */
	uint8_t		*packed_key;	/* Packed key for 'raw' interface */
	uint8_t		*packed_value;	/* Packed value for 'raw' interface */
	uint32_t	step_count;	/* Intra-record count */
	uint32_t	rectype;	/* Record type */
	uint64_t	txnid;		/* Record txnid */
	uint32_t	flags;
};

struct __wt_cursor_metadata {
	WT_CURSOR iface;

	WT_CURSOR *file_cursor;		/* Queries of regular metadata */

#define	WT_MDC_CREATEONLY	0x01
#define	WT_MDC_ONMETADATA	0x02
#define	WT_MDC_POSITIONED	0x04
	uint32_t flags;
};

struct __wt_join_stats_group {
	const char *desc_prefix;	/* Prefix appears before description */
	WT_CURSOR_JOIN *join_cursor;
	ssize_t join_cursor_entry;	/* Position in entries */
	WT_JOIN_STATS join_stats;
};

struct __wt_cursor_stat {
	WT_CURSOR iface;

	bool	notinitialized;		/* Cursor not initialized */
	bool	notpositioned;		/* Cursor not positioned */

	int64_t	     *stats;		/* Statistics */
	int	      stats_base;	/* Base statistics value */
	int	      stats_count;	/* Count of statistics values */
	int	     (*stats_desc)(WT_CURSOR_STAT *, int, const char **);
					/* Statistics descriptions */
	int	     (*next_set)(WT_SESSION_IMPL *, WT_CURSOR_STAT *, bool,
			 bool);		/* Advance to next set */

	union {				/* Copies of the statistics */
		WT_DSRC_STATS dsrc_stats;
		WT_CONNECTION_STATS conn_stats;
		WT_JOIN_STATS_GROUP join_stats_group;
	} u;

	const char **cfg;		/* Original cursor configuration */
	char	*desc_buf;		/* Saved description string */

	int	 key;			/* Current stats key */
	uint64_t v;			/* Current stats value */
	WT_ITEM	 pv;			/* Current stats value (string) */

	/* Uses the same values as WT_CONNECTION::stat_flags field */
	uint32_t flags;
};

/*
 * WT_CURSOR_STATS --
 *	Return a reference to a statistic cursor's stats structures.
 */
#define	WT_CURSOR_STATS(cursor)						\
	(((WT_CURSOR_STAT *)cursor)->stats)

struct __wt_cursor_table {
	WT_CURSOR iface;

	WT_TABLE *table;
	const char *plan;

	const char **cfg;		/* Saved configuration string */

	WT_CURSOR **cg_cursors;
	WT_ITEM *cg_valcopy;		/*
					 * Copies of column group values, for
					 * overlapping set_value calls.
					 */
	WT_CURSOR **idx_cursors;
};

#define	WT_CURSOR_PRIMARY(cursor)					\
	(((WT_CURSOR_TABLE *)cursor)->cg_cursors[0])

#define	WT_CURSOR_RECNO(cursor)	WT_STREQ((cursor)->key_format, "r")

/*
 * WT_CURSOR_NEEDKEY, WT_CURSOR_NEEDVALUE --
 *	Check if we have a key/value set.  There's an additional semantic
 * implemented here: if we're pointing into the tree, and about to perform
 * a cursor operation, get a local copy of whatever we're referencing in
 * the tree, there's an obvious race with the cursor moving and the key or
 * value reference, and it's better to solve it here than in the underlying
 * data-source layers.
 *
 * WT_CURSOR_CHECKKEY --
 *	Check if a key is set without making a copy.
 *
 * WT_CURSOR_NOVALUE --
 *	Release any cached value before an operation that could update the
 * transaction context and free data a value is pointing to.
 */
#define	WT_CURSOR_CHECKKEY(cursor) do {					\
	if (!F_ISSET(cursor, WT_CURSTD_KEY_SET))			\
		WT_ERR(__wt_cursor_kv_not_set(cursor, true));		\
} while (0)
#define	WT_CURSOR_CHECKVALUE(cursor) do {				\
	if (!F_ISSET(cursor, WT_CURSTD_VALUE_SET))			\
		WT_ERR(__wt_cursor_kv_not_set(cursor, false));		\
} while (0)
#define	WT_CURSOR_NEEDKEY(cursor) do {					\
	if (F_ISSET(cursor, WT_CURSTD_KEY_INT)) {			\
		if (!WT_DATA_IN_ITEM(&(cursor)->key))			\
			WT_ERR(__wt_buf_set(				\
			    (WT_SESSION_IMPL *)(cursor)->session,	\
			    &(cursor)->key,				\
			    (cursor)->key.data, (cursor)->key.size));	\
		F_CLR(cursor, WT_CURSTD_KEY_INT);			\
		F_SET(cursor, WT_CURSTD_KEY_EXT);			\
	}								\
	WT_CURSOR_CHECKKEY(cursor);					\
} while (0)
#define	WT_CURSOR_NEEDVALUE(cursor) do {				\
	if (F_ISSET(cursor, WT_CURSTD_VALUE_INT)) {			\
		if (!WT_DATA_IN_ITEM(&(cursor)->value))			\
			WT_ERR(__wt_buf_set(				\
			    (WT_SESSION_IMPL *)(cursor)->session,	\
			    &(cursor)->value,				\
			    (cursor)->value.data, (cursor)->value.size));\
		F_CLR(cursor, WT_CURSTD_VALUE_INT);			\
		F_SET(cursor, WT_CURSTD_VALUE_EXT);			\
	}								\
	WT_CURSOR_CHECKVALUE(cursor);					\
} while (0)
#define	WT_CURSOR_NOVALUE(cursor) do {					\
	F_CLR(cursor, WT_CURSTD_VALUE_INT);				\
} while (0)

#define	WT_CURSOR_RAW_OK						\
	WT_CURSTD_DUMP_HEX | WT_CURSTD_DUMP_PRINT | WT_CURSTD_RAW
