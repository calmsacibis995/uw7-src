/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:userad/format.c	1.1"
#endif
/*
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

#include "LoginMgr.h"

/* operation_num 0 = create */
/* operation_num 1 = modify */
/* operation_num 2 = delete */
/* operation_num 3 = remove ownership from */
/* operation_num 4 = add ownership to */

/* context_num 0 = user account */
/* context_num 1 = desktop environment */
/* context_num 2 = group */
/* context_num 3 = reserved account */

char * GetMessage_Fmt(int op_num, int cont_num, int op_stat)
{
   /* This routine attempts to get the correct format based on */
   /* operation_num, context_num and operation status. */

    switch(op_num)
    {                 /* operation switch */ 
    case 0:
          /* create */
          switch(cont_num)
          {
          case 0:
                /* user account */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                    return (format_createUserAcc1);
                    break;
               case 1:
                     /* ? */
                    return (format_createUserAcc2);
                    break;
               case 2:
                     /* succeeded */
                    return (format_createUserAcc3);
                    break;
               case 3:
                     /* failed */
                    return (format_createUserAcc4);
                    break;
               case 4:
                     /* unchanged */
                    return (format_createUserAcc5);
                    break;
               default:
                    break;
               }

          case 1:
                /* desktop environment */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_createDeskEnv1);
                     break;
               case 1:
                     /* ? */
                     return (format_createDeskEnv2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_createDeskEnv3);
                     break;
               case 3:
                     /* failed */
                     return (format_createDeskEnv4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_createDeskEnv5);
                     break;
               default:
                    break;
               }
          case 2:
                /* group */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_createGroup1);
                     break;
               case 1:
                     /* ? */
                     return (format_createGroup2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_createGroup3);
                     break;
               case 3:
                     /* failed */
                     return (format_createGroup4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_createGroup5);
                     break;
               default:
                    break;
               }
          case 3:
                /* reserved account */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_createResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_createResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_createResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_createResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_createResAcc5);
                     break;
               default:
                    break;
               }
          } /* cont_num switch */ 
    case 1:
          /* modify */
          switch(cont_num)
          {
          case 0:
                /* user account */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_modifyUserAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_modifyUserAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_modifyUserAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_modifyUserAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_modifyUserAcc5);
                     break;
               default:
                    break;
               }
          case 1:
                /* desktop environment */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_modifyDeskEnv1);
                     break;
               case 1:
                     /* ? */
                     return (format_modifyDeskEnv2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_modifyDeskEnv3);
                     break;
               case 3:
                     /* failed */
                     return (format_modifyDeskEnv4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_modifyDeskEnv5);
                     break;
               default:
                    break;
               }
          case 2:
                /* group */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_modifyGroup1);
                     break;
               case 1:
                     /* ? */
                     return (format_modifyGroup2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_modifyGroup3);
                     break;
               case 3:
                     /* failed */
                     return (format_modifyGroup4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_modifyGroup5);
                     break;
               default:
                    break;
               }
          case 3:
                /* reserved account */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_modifyResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_modifyResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_modifyResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_modifyResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_modifyResAcc5);
                     break;
               default:
                    break;
               }
          } /* cont_num */
   case 2:
         /* delete */
         switch(cont_num)
         {
         case 0:
                /* user account */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_deleteUserAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_deleteUserAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_deleteUserAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_deleteUserAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_deleteUserAcc5);
                     break;
               default:
                    break;
               }
         case 1:
                /* desktop environment */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_deleteDeskEnv1);
                     break;
               case 1:
                     /* ? */
                     return (format_deleteDeskEnv2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_deleteDeskEnv3);
                     break;
               case 3:
                     /* failed */
                     return (format_deleteDeskEnv4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_deleteDeskEnv5);
                     break;
               default:
                    break;
               }
         case 2:
                /* group */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_deleteGroup1);
                     break;
               case 1:
                     /* ? */
                     return (format_deleteGroup2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_deleteGroup3);
                     break;
               case 3:
                     /* failed */
                     return (format_deleteGroup4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_deleteGroup5);
                     break;
               default:
                    break;
               }
         case 3:
                /* reserved account */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_deleteResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_deleteResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_deleteResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_deleteResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_deleteResAcc5);
                     break;
               default:
                    break;
               }
         } /* cont_num */
   case 3:
        /* remove ownership from */
        switch(cont_num)
        {
        case 0:
                /* user account */
              switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_delOwnerUserAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_delOwnerUserAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_delOwnerUserAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_delOwnerUserAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_delOwnerUserAcc5);
                     break;
               default:
                    break;
               }

        case 3:
                /* reserved account */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_delOwnerResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_delOwnerResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_delOwnerResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_delOwnerResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_delOwnerResAcc5);
                     break;
               default:
                    break;
               }
        } /* cont_num */
   case 4:
        /* add ownership to */
        switch(cont_num)
        {
        case 0:
                /* user account */
              switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_addOwnerUserAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_addOwnerUserAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_addOwnerUserAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_addOwnerUserAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_addOwnerUserAcc5);
                     break;
               default:
                    break;
               }
        case 3:
                /* reserved account */
              switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_addOwnerResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_addOwnerResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_addOwnerResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_addOwnerResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_addOwnerResAcc5);
                     break;
               default:
                    break;
               }

        } /* cont_num */

    } /* op_num switch */ 

} /* GetMessage_Fmt() */
