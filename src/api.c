/* api.c - functions called only by modules */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"	/* required by mushconf */
#include "game.h"		/* required by mushconf */
#include "alloc.h"		/* required by mushconf */
#include "flags.h"		/* required by mushconf */
#include "htab.h"		/* required by mushconf */
#include "libltdttdl.h"		/* required by mushconf */
#include "udb.h"		/* required by mushconf */
#include "udb_defs.h"	/* required by mushconf */

#include "mushconf.h"	/* required by code */

#include "db.h"			/* required by externs */
#include "interface.h"		/* required by code */
#include "externs.h"		/* required by interface */

#include "command.h"		/* required by code */
#include "match.h"		/* required by code */
#include "vattr.h"		/* required by code */
#include "attrs.h"		/* required by code */
#include "powers.h"		/* required by code */
#include "functions.h"		/* required by code */
#include "udb.h"		/* required by code */
#include "udb_defs.h"		/* required by code */

extern CMDENT *prefix_cmds[256];

/*
 * ---------------------------------------------------------------------------
 * Exporting a module's own API.
 */

void register_api(char *module_name, char *api_name, API_FUNCTION * ftable) {
	MODULE *mp;
	API_FUNCTION *afp;
	void (*fn_ptr)(void *, void *);
	int succ = 0;
	char s[MBUF_SIZE];

	for (mp = mudstate.modules_list; mp != NULL; mp = mp->next) {
		if (!strcmp(module_name, mp->modname)) {
			succ = 1;
			break;
		}
	}

	if (!succ) { /* no such module */
		return;
	}

	for (afp = ftable; afp->name; afp++) {
		snprintf(s, MBUF_SIZE, "mod_%s_%s", module_name, afp->name);
		fn_ptr = (void (*)(void *, void *)) lt_dlsym(mp->handle, s);

		if (fn_ptr != NULL) {
			afp->handler = fn_ptr;
			snprintf(s, MBUF_SIZE, "%s_%s", api_name, afp->name);
			hashadd(s, (int * ) afp, &mudstate.api_func_htab, 0);
		}
	}
}

void *request_api_function(char *api_name, char *fn_name) {
	API_FUNCTION *afp;
	char s[MBUF_SIZE];
	snprintf(s, MBUF_SIZE, "%s_%s", api_name, fn_name);
	afp = (API_FUNCTION *) hashfind(s, &mudstate.api_func_htab);

	if (!afp) {
		return NULL;
	}

	return afp->handler;
}

/*
 * ---------------------------------------------------------------------------
 * Handle tables.
 */

void register_commands(CMDENT * cmdtab) {
	CMDENT *cp;
	char s[MBUF_SIZE];

	if (cmdtab) {
		for (cp = cmdtab; cp->cmdname; cp++) {
			hashadd(cp->cmdname, (int * ) cp, &mudstate.command_htab, 0);
			snprintf(s, MBUF_SIZE, "__%s", cp->cmdname);
			hashadd(s, (int * ) cp, &mudstate.command_htab, HASH_ALIAS);
		}
	}
}

void register_prefix_cmds(const char *cmdchars) {
	const char *cp;
	char cn[2] = "x";

	if (cmdchars) {
		for (cp = cmdchars; *cp; cp++) {
			cn[0] = *cp;
			prefix_cmds[(unsigned char) *cp] = (CMDENT *) hashfind(cn,
					&mudstate.command_htab);
		}
	}
}

void register_functions(FUN * functab) {
	FUN *fp;

	if (functab) {
		for (fp = functab; fp->name; fp++) {
			hashadd((char * ) fp->name, (int * ) fp, &mudstate.func_htab, 0);
		}
	}
}

void register_hashtables(MODHASHES * htab, MODNHASHES * ntab) {
	MODHASHES *hp;
	MODNHASHES *np;

	if (htab) {
		for (hp = htab; hp->tabname != NULL; hp++) {
			hashinit(hp->htab, hp->size_factor * mudconf.hash_factor, HT_STR);
		}
	}

	if (ntab) {
		for (np = ntab; np->tabname != NULL; np++) {
			nhashinit(np->htab, np->size_factor * mudconf.hash_factor);
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * Deal with additional database info.
 */

unsigned int register_dbtype(char *modname) {
	unsigned int type;
	DBData key, data;
	/*
	 * Find out if the module already has a registered DB type
	 */
	key.dptr = modname;
	key.dsize = strlen(modname) + 1;
	data = db_get(key, DBTYPE_MODULETYPE);

	if (data.dptr) {
		memcpy((void *) &type, (void *) data.dptr, sizeof(unsigned int));
		xfree(data.dptr, "register_dbtype");
		return type;
	}

	/*
	 * If the type is in range, return it, else return zero as an error
	 * code
	 */

	if ((mudstate.moduletype_top >= DBTYPE_RESERVED)
			&& (mudstate.moduletype_top < DBTYPE_END)) {
		/*
		 * Write the entry to GDBM
		 */
		data.dptr = &mudstate.moduletype_top;
		data.dsize = sizeof(unsigned int);
		db_put(key, data, DBTYPE_MODULETYPE);
		type = mudstate.moduletype_top;
		mudstate.moduletype_top++;
		return type;
	} else {
		return 0;
	}
}
