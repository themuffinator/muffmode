//===========================================================================
//
// Name:				bl_botcfg.h
// Function:		bot configuration files
// Programmer:		Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:	1998-01-12
// Tab Size:		3
//===========================================================================

typedef struct bot_s {
	char name[MAX_QPATH];
	char skin[MAX_QPATH];
	char charfile[MAX_QPATH];
	char charname[MAX_QPATH];
	struct bot_s *next;
} bot_t;

extern bot_t *botlist;

void AppendPathSeperator(char *path, int length);
bot_t *FindBotWithName(char *name);
void CheckForNewBotFile(void);
int AddRandomBot(gentity_t *ent);
