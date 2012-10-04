
#include "g_local.h"


void	Svcmd_Test_f (void)
{
#ifdef WITH_ACEBOT
			safe_cprintf
#else
			gi.cprintf 
#endif
	 (NULL, PRINT_HIGH, "Svcmd_Test_f()\n");
}

/*
=================
ServerCommand

ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() / gi.argv() commands to get the rest
of the parameters
=================
*/
void	ServerCommand (void)
{
	char	*cmd;

	cmd = gi.argv(1);
	if (Q_stricmp (cmd, "test") == 0)
		Svcmd_Test_f ();
#ifdef WITH_ACEBOT
// ACEBOT_ADD
	else if(Q_stricmp (cmd, "acedebug") == 0)
 		if (strcmp(gi.argv(2),"on")==0)
		{
			safe_bprintf (PRINT_MEDIUM, "ACE: Debug Mode On\n");
			debug_mode = true;
		}
		else
		{
			safe_bprintf (PRINT_MEDIUM, "ACE: Debug Mode Off\n");
			debug_mode = false;
		}
	else if (Q_stricmp (cmd, "addbot") == 0)
		ACESP_SpawnBot (NULL, gi.argv(2), gi.argv(3), NULL);
	// removebot
	else if(Q_stricmp (cmd, "removebot") == 0)
    	ACESP_RemoveBot(gi.argv(2));
	
	// Node saving
	else if(Q_stricmp (cmd, "savenodes") == 0)
    	ACEND_SaveNodes();
// ACEBOT_END
#endif
	else
#ifdef WITH_ACEBOT
			safe_cprintf
#else
			gi.cprintf 
#endif
		 (NULL, PRINT_HIGH, "Unknown server command \"%s\"\n", cmd);
}

