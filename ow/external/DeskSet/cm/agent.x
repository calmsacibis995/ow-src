/*
   static  char sccsid[] = "@(#)agent.x 3.2 92/07/07 Copyr 1991 Sun Microsystems, Inc.";

   agent.x

#ifdef RPC_XDR
%#include "rtable.h"
#endif

*/

enum Update_Status {
	update_succeeded=0,
	update_failed=1
};

%extern void init_agent();

%extern void destroy_agent();

/*	The AGENTPROG actually isn't used for callback.
	A transient number will be generated instead.  It's
	just declared here as a "syntax" holder for rpcgen
*/
program AGENTPROG {
        version AGENTVERS {
                Update_Status update_callback(Table_Res) = 1;
        }=1;
}=00;
