#ident	"@(#)pdi.cmds:sdipath.c	1.1"

/*
 ********************************************************************
 *
 *  Module:
 *
 *      sdipath.c
 *
 *  Description:
 *
 *      This module contains all of the definitions and code that
 *      comprises the sdipath command. This command is used to
 *      manage some of the Multi Path I/O operations that users
 *      can perform. These operations consist of the following:
 *
 *      list
 *          This operation lists information about the paths that
 *          are associated to a particular device or about all paths
 *          that the MPIO driver knows about.
 *
 *      fail
 *          This operation changes the state of the specified path
 *          to "failed". This will prevent I/O from travelling on
 *          this path.
 *
 *      repair
 *          This operation is used to indicate that a specified path
 *          has been repaired.
 *
 *      switch
 *          This operation switches I/O from one CPU group to another.
 *
 *      error
 *          This is an undocumented operation that will instruct the
 *          MPIO driver to insert or remove an error on a particular
 *          path.
 *
 *  Notes:
 *
 *      None.
 *
 *******************************************************************
 */

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <locale.h>
#include <pfmt.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/resmgr.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <devmgmt.h>
#include <sys/mpio_ioctl.h>


/*
 *  Miscellaneous definitions for sdipath command.
 */

#define SDIP_TRUE            (1)
#define SDIP_FALSE           (0)

#define SDIP_SUCCESS         (0)
#define SDIP_FAILURE         (-1)

#define SDIP_INVALID_LOCALE  (-1)

#define SDIP_8               (8)
#define SDIP_32              (32)
#define SDIP_64              (64)
#define SDIP_256             (256)

#define SDIP_DISK_DIR        "/dev/rdsk"


/*
 *  Define message strings that this command will display.
 */
#define MSG_ERR_FAIL    ":1:Failing %s errno %d.\n"
#define MSG_ERR_REPAIR  ":2:Repairing %s, errno %d.\n"
#define MSG_ERR_SWITCH  ":3:Switching locale for %s, errno %d.\n"


#define MSG_OK_FAIL     ":4:Path %s has been failed.\n"
#define MSG_OK_REPAIR   ":5:Path %s has been repaired.\n"
#define MSG_OK_SWITCH   ":6:Device %s has been switched to locale %d.\n"


#define MSG_NOARG       ":7:Argument required for %s operation.\n"
#define MSG_NOOP        ":8:Operation must be specified.\n"
#define MSG_NODEV       ":9:Device required for %s operation.\n"
#define MSG_BADOP       ":10:Invalid operation %s specified.\n"
#define MSG_BADOPEN     ":11:Open of %s failed, errno %d.\n"

#define MSG_NO_PATH     ":12:Could not find %s for %s.\n"
#define MSG_GET_PATHS   ":13:Getting paths for device %s, errno %d.\n"
#define MSG_GET_INFO    ":14:Getting device %s info for path%d, errno %d.\n"
#define MSG_NOLOCALE    ":15:%s locale required for switch operation.\n"


/*
 *  Forward declarations of non integer returning functions.
 */
void sdipath_usage( char    *format, ... );

void sdipath_print( int                             list_mode,
                    char                            *device,
                    mpio_ioctl_get_path_info_t      *info_ptr );


/*
 *  Global variables.
 */
char        *global_progname    = NULL;
char        *global_label       = NULL;

int         global_need_heading = SDIP_TRUE;


/*
 *  Enumeration of sdipath command operations.
 */
enum {
    Sdipath_List,
    Sdipath_Fail,
    Sdipath_Repair,
    Sdipath_Switch,
    Sdipath_Reset,
    Sdipath_Error
};


/*
 *  Enumeration of list operation modes.
 */
enum {
    List_Verbose,
    List_Quiet
};


/*
 *  Enumeration of MPIO path states. This enumeration is a
 *  redefinition of the one in the MPIO driver. The one in the
 *  driver is not located in a header that could be easily included
 *  here. If the enum in the driver changes, this should too.
 */
enum {
    Sdipath_Active,
    Sdipath_Inactive,
    Sdipath_Failed
};


/*
 *  Enumeration of sdipath exit codes.
 */
enum {
    SDIP_Exit_Ok,
    SDIP_Exit_Error,
    SDIP_Exit_Access,
    SDIP_Exit_Usage
};

/*
 * *******************************************************************
 *
 *  Name:
 *
 *      main()
 *
 *  Description:
 *
 *      This is the main function for the sdipath command. This
 *      function parses the command line and determines which
 *      operation routine should be called to process the specified
 *      operation.
 *
 *  Parameters:
 *
 *      int                 argc
 *                          Number of arguments in command argv.
 *
 *      char                *argv[]
 *                          Array of command line arguments.
 *
 *  Return Values:
 *
 *      SDIP_Exit_Ok         Operation succeeded.
 *      SDIP_Exit_Error      Operation failed.
 *      SDIP_Exit_Usage      Program detected a usage error.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

main( int           argc,
      char          *argv[] )
{
    int             arg;
    int             loopcnt      = 0;

    int             d_locale     = SDIP_INVALID_LOCALE;
    int             s_locale     = SDIP_INVALID_LOCALE;

    int             insert_error = SDIP_FALSE;
    int             remove_error = SDIP_FALSE;
    
    char            *device      = NULL;
    char            *operation   = NULL;

    int             list_mode    = List_Verbose;

    int             exit_status  = 0;

    char            *arguments[ SDIP_256 ];
    int             arg_cnt      = 0;
    
/**********************************************************************/

    /*
     *  Set up the program name, the locale, the message catalog and
     *  the program label for pfmt(3C).
     */
    global_progname = basename( argv[ 0 ] );

    ( void ) setlocale( LC_ALL, "" );
    ( void ) setcat( "sdipath" );
    
    global_label =
        ( char * ) malloc( strlen( global_progname ) + 1 + 3 );
    sprintf( global_label, "UX:%s", global_progname );
    ( void ) setlabel( global_label );

    /*
     *  Initialize our internal argument vector. This will be passed
     *  to the operation specific routines after the command line is
     *  deemed to be correct.
     */
    memset( &arguments, 0, sizeof( arguments ) );

    /*
     *  Now parse the command line to see what the user specified.
     *  Validating the options will be done after it is parsed.
     */
    while( ( arg = getopt( argc, argv, "o:qvirS:d:D:" ) ) != EOF )
    {
        switch( arg )
        {
          case 'd':
            device = strdup( optarg );
            break;

          case 'o':
            operation = strdup( optarg );
            break;

          case 'q':
            list_mode           = List_Quiet;
            global_need_heading = SDIP_FALSE;
            break;

          case 'S':
            s_locale = atoi( optarg );
            break;

          case 'D':
            d_locale = atoi( optarg );
            break;
            
          case 'v':
            list_mode = List_Verbose;
            break;

          case 'i':
            insert_error = SDIP_TRUE;
            remove_error = SDIP_FALSE;
            break;

          case 'r':
            remove_error = SDIP_TRUE;
            insert_error = SDIP_FALSE;
            break;
            
          case '?' : /* Incorrect argument found */
            sdipath_usage( "" );
            break;
        }
    }

    /*
     *  Load our internal argument vector with everything left
     *  on the command line. The operation routines will determine
     *  if this information is sane.
     */
    for ( loopcnt = optind; loopcnt < argc; loopcnt++ )
    {
        arguments[ arg_cnt ] = argv[ loopcnt ];
        arg_cnt++;
    }

    /*
     *  Now make sure we have a valid operation. If so, call the
     *  operation specific routine to process the request. If not,
     *  issue a usage message.
     */
    if  ( 0 == strcmp( "list", operation ) )
    {
        exit_status = sdipath_list( device,
                                    list_mode );
    }

    else if  ( 0 == strcmp( "fail", operation ) ) 
    {
        exit_status = sdipath_manage( Sdipath_Fail,
                                      device,
                                      arg_cnt,
                                      arguments );
    }

    else if  ( 0 == strcmp( "repair", operation ) )
    {
        exit_status = sdipath_manage( Sdipath_Repair,
                                      device,
                                      arg_cnt,
                                      arguments );
    }

    else if  ( 0 == strcmp( "switch", operation ) )
    {
        exit_status = sdipath_switch( device,
                                      s_locale,
                                      d_locale );
    }

    else if  ( 0 == strcmp( "error", operation ) )
    {
        exit_status = sdipath_error( device,
                                     insert_error,
                                     remove_error,
                                     arg_cnt,
                                     arguments );
    }

    else if  ( 0 == strcmp( "reset", operation ) )
    {
        exit_status = sdipath_reset( );
    }
    
    else
    {
        /*
         *  Differentiate (via error message) between an invalid
         *  operation and no operation being specified.
         */
        if  ( operation == NULL )
        {
            sdipath_usage( MSG_NOOP );
        }
        else
        {
            sdipath_usage( MSG_BADOP, operation );
        }
    }

    /*
     *  See if the operation returned with a SDIP_Exit_Usage status.
     *  We do this because some of our operation routines do
     *  processing to ensure all arguments are properly satisfied.
     *
     *  Otherwise, return the status of operation specific routine.
     */
    if  ( exit_status == SDIP_Exit_Usage )
    {
        sdipath_usage( "" );
    }

    exit( exit_status );
    
} /* main() */

/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_list()
 *
 *  Description:
 *
 *      This function manages the list operation. This operation
 *      lists information about paths that the MPIO driver knows
 *      about. The format of the information produced depends on
 *      the list option (one of "qv") specified on the command line.
 *
 *      If the device argument is NULL then information about all
 *      devices that MPIO knows about will be displayed. If the
 *      device argument is not NULL then only information about the
 *      specified device will be displayed.
 *
 *  Parameters:
 *
 *      char            *device
 *                      Name of device to list info for.
 *
 *      int             list_mode
 *                      Mode (verbose or quiet) for listing.
 *
 *  Return Values:
 *
 *      SDIP_Exit_Ok     Function completed without error.
 *      SDIP_Exit_Error  Function encountered an error.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

int
sdipath_list( char      *device,
              int       list_mode )
{
    int                 length       = 0;
    int                 return_error = SDIP_FALSE;
    int                 status       = SDIP_Exit_Ok;
    
    DIR                 *dirp        = NULL;
    dirent_t            *dir_entry   = NULL;

    char                filename[ SDIP_64 ];

    char                buffer[ SDIP_64 ];
    char                **dev_list = NULL;
    char                *criteria_list[ 2 ];
    int                 done = 0;
    int                 val = 1;
    
/*********************************************************************/

    /*
     *  If the device is null, then read through the /etc/device.tab
     *  file for all of the disk devices.
     */
    if  ( device == NULL )
    {
        while ( !done )
        {
            sprintf( buffer, "alias=disk%d", val );
            criteria_list[ 0 ] = buffer;
            criteria_list[ 1 ] = NULL;

            dev_list = getdev( ( char ** ) NULL,
                               criteria_list,
                               DTAB_ANDCRITERIA );
            if  ( *dev_list == NULL )
            {
                done = SDIP_TRUE;
            }
            else
            {
                status = sdipath_list_for_device( *dev_list,
                                                  list_mode );
                if  ( status != SDIP_Exit_Ok )
                {
                    /*
                     *  Specific error message displayed from
                     *  called routine, just return the error
                     *  status here.
                     */
                    return_error = SDIP_TRUE;
                }
            }

            val++;
        }
    }
    
    else
    {
        /*
         *  The device is being specified so pass it on.
         */
        status = sdipath_list_for_device( device, list_mode );
        
        /*
         *  Specific error message displayed from called routine,
         *  just return the error status here.
         */
    }
    
    return( status );
    
} /* sdipath_list() */


/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_list_for_device()
 *
 *  Description:
 *
 *      This routine controls the list operation for a specified
 *      device. The device is found, opened, and then the all of
 *      the information about its paths is retrieved, formatted,
 *      and displayed in the requested form (quiet or verbose).
 *
 *  Parameters:
 *
 *      char            *device
 *                      Name of device to open and get info for.
 *
 *      int             list_mode
 *                      Mode (quiet or verbose) for listing.
 *
 *  Return Values:
 *
 *      SDIP_Exit_Ok     Function completed without error.
 *      SDIP_Exit_Error  Function encountered an error.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

int
sdipath_list_for_device( char       *device,
                         int        list_mode )
{
    int                         devicefd;
    int                         i;
    int                         status = SDIP_Exit_Ok;
    int                         return_error = SDIP_FALSE;

    mpio_ioctl_get_paths_t      get_paths_pkt;
    mpio_ioctl_get_path_info_t  path_info_pkt;

    rm_key_t                    rmkey;
    rm_key_t                    *rmkeys;
    
/*********************************************************************/

    /*
     *  Open the specified device. This function will find the actual
     *  device path and open it.
     */
    status = sdipath_open_device( device, &devicefd );
    if  ( status == SDIP_Exit_Error )
    {
        /*
         *  Specific error message displayed from open routine, just
         *  return the error status here.
         */
        return( status );
    };

    /*
     *  Issue the ioctl to see how many paths this device has.
     */
    get_paths_pkt.version    = MPIO_IOCTL_VERSION_1;
    get_paths_pkt.num_input  = 0;
    get_paths_pkt.input      = 0;
    get_paths_pkt.num_output = 0;
    
    status = ioctl( devicefd, MPIO_IOCTL_GET_PATHS, &get_paths_pkt );
    /*
     *  This call returns an error but gives the correct number of
     *  paths in the packet, so ignore the return status and use the
     *  value in the packet.
     */
    if  ( ( status != SDIP_FAILURE ) || ( errno != EAGAIN ) )
    {
        printf( "MPIO_IOCTL_GET_PATHS() status %d, errno %d, output %d\n",
                status, errno, get_paths_pkt.num_output );
        pfmt( stderr, MM_ERROR, MSG_GET_PATHS, device, errno );
        close( devicefd );
        return( SDIP_Exit_Error );
    }

    /*
     *  Issue the ioctl to get all of the resmgr keys for all the
     *  paths for this device.
     */
    get_paths_pkt.version   = MPIO_IOCTL_VERSION_1;
    get_paths_pkt.num_input = get_paths_pkt.num_output;

    rmkeys = ( rm_key_t * )
        malloc( sizeof( rm_key_t ) * get_paths_pkt.num_output );
    get_paths_pkt.input     = rmkeys;
    
    status = ioctl( devicefd, MPIO_IOCTL_GET_PATHS, &get_paths_pkt );
    if  ( status != SDIP_Exit_Ok )
    {
        pfmt( stderr, MM_ERROR, MSG_GET_PATHS, device, errno );
        close( devicefd );
        return( status );
    }

    /*
     *  Loop through the array of resmgr keys that were returned and
     *  issue the ioctl to get information about each path.
     */
    for ( i = 0; i < get_paths_pkt.num_output; i++ )
    {
        path_info_pkt.version    = MPIO_IOCTL_VERSION_1;
        path_info_pkt.path_rmkey = rmkeys[ i ];

        status = ioctl( devicefd,
                        MPIO_IOCTL_GET_PATH_INFO,
                        &path_info_pkt );

        if  ( status == SDIP_Exit_Ok )
        {
            /*
             *  Call the function to format and print the information.
             */
            ( void )sdipath_print( list_mode,
                                   device,
                                   &path_info_pkt );
        }
        else
        {
            pfmt( stderr, MM_ERROR, MSG_GET_INFO,
                  device, rmkeys[ i ], errno );
            return_error = SDIP_TRUE;
        }
    }

    /*
     *  Close the device and return our status.
     */
    close( devicefd );

    if  ( return_error )
    {
        return( SDIP_Exit_Error );
    }
    else
    {
        return( SDIP_Exit_Ok );
    }

} /* sdipath_list_for_device() */


/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_print()
 *
 *  Description:
 *
 *      This function formats and prints the information about the
 *      specified path.
 *
 *  Parameters:
 *
 *      int             list_mode
 *                      The mode (quiet or verbose) to format the
 *                      list output.
 *
 *      char            *device
 *                      The name of the device this path is
 *                      associated with.
 *
 *      mpio_ioctl_get_path_info_t  *info_ptr
 *                      Pointer to structure containing all of the
 *                      MPIO information for this path.
 *
 *  Return Values:
 *
 *      None.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

void
sdipath_print( int                          list_mode,
               char                         *device,
               mpio_ioctl_get_path_info_t   *info_ptr )
{
    char            *path_state = "Unknown";
    int             status = SDIP_Exit_Ok;
    char            tmpname[ SDIP_32 ];

/*********************************************************************/

    /*
     *  See if we need to print a heading.
     */
    if  ( global_need_heading && ( list_mode == List_Verbose ) )
    {
        /*
         *  Print the heading and then turn off the global variable
         *  so we don't print it again.
         */
        fprintf( stdout,
                 "%-14s %-8s %-10s %-12s %-11s %-9s\n",
                 "   Device   ", " Path ", " State  ", "CPU Group",
                 "  Reads  ", " Writes" );
        fprintf( stdout,
                 "%-14s %-8s %-10s %-12s %-11s %-9s\n",
                 "------------", "------", "--------", "---------",
                 "---------", "---------" );

        global_need_heading = SDIP_FALSE;
    }

    /*
     *  Convert the integer state value to text.
     */
    switch ( info_ptr->state )
    {
      case Sdipath_Active:
        path_state = "Active";
        break;

      case Sdipath_Inactive:
        path_state = "Inactive";
        break;

      case Sdipath_Failed:
        path_state = "Failed";
        break;

      default:
        path_state = "Unknown";
    }

    /*
     *  Build up the path name string based on the paths resmgr key.
     */
    sprintf( tmpname, "path%d", info_ptr->path_rmkey );

    /*
     *  Print the line according to the desired format.
     */
    switch ( list_mode )
    {
      case List_Verbose:
        fprintf( stdout,
                 "%-14s %-8s %-10s     %-8d %9d   %9d\n",
                 basename( device ),
                 tmpname,
                 path_state,
                 info_ptr->locale,
                 info_ptr->num_reads,
                 info_ptr->num_writes );
        break;
        
      case List_Quiet:
        fprintf( stdout,
                 "%s:%s:%s:%d:%d:%d\n",
                 device,
                 tmpname,
                 path_state,
                 info_ptr->locale,
                 info_ptr->num_reads,
                 info_ptr->num_writes );
        break;

      default:
        break;
    }

} /* sdipath_print() */

/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_manage()
 *
 *  Description:
 *
 *      This function manages the four operations (fail, and repair)
 *      that simply issue ioctl's to the driver to perform the
 *      appropriate function.
 *
 *  Parameters:
 *
 *      int             opertion
 *                      Value indicating which operation is being
 *                      performed.
 *
 *      char            *device
 *                      Name of device to apply operation to.
 *
 *      int             arg_cnt
 *                      Number of elements in arguments array.
 *
 *      char            **arguments
 *                      Array of argument values to process.
 *
 *  Return Values:
 *
 *      SDIP_Exit_Ok     Function completed without error.
 *      SDIP_Exit_Error  Function encountered an error.
 *      SDIP_Exit_Usage  Function detected a usage error.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

int
sdipath_manage( int         operation,
                char        *device,
                int         arg_cnt,
                char        **arguments )
{
    char                    *ioctl_err_msg = NULL;
    char                    *ioctl_ok_msg  = NULL;
    char                    *noarg_op      = NULL;

    char                    tmp_path[ SDIP_32 ];

    int                     devicefd       = 0;
    int                     ioctl_cmd      = 0;
    int                     i, j;
    int                     return_error   = SDIP_FALSE;
    int                     status         = SDIP_Exit_Ok;
    int                     found          = 0;
    
    mpio_ioctl_path_t       path_pkt;
    mpio_ioctl_get_paths_t  get_paths_pkt;

    rm_key_t                rmkey;
    rm_key_t                *rmkeys;
    
/*********************************************************************/

    /*
     *  Based on which operation we are performing, setup the ioctl
     *  command value and the associated messages.
     */
    switch ( operation )
    {
      case Sdipath_Fail:
        ioctl_cmd     = MPIO_IOCTL_FAIL_PATH;
        ioctl_err_msg = MSG_ERR_FAIL;
        ioctl_ok_msg  = MSG_OK_FAIL;
        noarg_op      = "fail";
        break;

      case Sdipath_Repair:
        ioctl_cmd     = MPIO_IOCTL_REPAIR_PATH;
        ioctl_err_msg = MSG_ERR_REPAIR;
        ioctl_ok_msg  = MSG_OK_REPAIR;
        noarg_op      = "repair";
        break;

      default:
        return( SDIP_Exit_Error );
    }

    /*
     *  Make sure there is a device specified.
     */
    if  ( device == NULL )
    {
        pfmt( stderr, MM_ERROR, MSG_NODEV, noarg_op );
        return( SDIP_Exit_Usage );
    }
    
    /*
     *  If there were no arguments passed to this function, print
     *  an error message and return with an SDIP_Exit_Usage status.
     */
    if  ( arg_cnt == 0 )
    {
        pfmt( stderr, MM_ERROR, MSG_NOARG, noarg_op );
        return( SDIP_Exit_Usage );
    }

    /*
     *  Open the device specified by the caller.
     */
    status = sdipath_open_device( device, &devicefd );
    if  ( status == SDIP_Exit_Error )
    {
        /*
         *  Specific error message displayed from open routine, just
         *  return the error status here.
         */
        return( status );
    }

    /*
     *  Issue the ioctl to see how many paths this device has.
     */
    get_paths_pkt.version    = MPIO_IOCTL_VERSION_1;
    get_paths_pkt.num_input  = 0;
    get_paths_pkt.input      = 0;
    get_paths_pkt.num_output = 0;
    
    status = ioctl( devicefd, MPIO_IOCTL_GET_PATHS, &get_paths_pkt );
    /*
     *  This call returns an error but gives the correct number of
     *  paths in the packet, so ignore the return status and use the
     *  value in the packet.
     */
    if  ( ( status != SDIP_FAILURE ) || ( errno != EAGAIN ) )
    {
        pfmt( stderr, MM_ERROR, MSG_GET_PATHS, device, errno );
        close( devicefd );
        return( SDIP_Exit_Error );
    }

    /*
     *  Issue the ioctl to get all of the resmgr keys for all the
     *  paths for this device.
     */
    get_paths_pkt.version   = MPIO_IOCTL_VERSION_1;
    get_paths_pkt.num_input = get_paths_pkt.num_output;

    rmkeys = ( rm_key_t * )
        malloc( sizeof( rm_key_t ) * get_paths_pkt.num_output );
    get_paths_pkt.input     = rmkeys;
    
    status = ioctl( devicefd, MPIO_IOCTL_GET_PATHS, &get_paths_pkt );
    if  ( status != SDIP_Exit_Ok )
    {
        pfmt( stderr, MM_ERROR, MSG_GET_PATHS, device, errno );
        return( status );
    }

    /*
     *  For each path that the user specified, get the device for the
     *  path name specified, then get the resource manager key for the
     *  path, fill out the packet and issue the ioctl.
     */
    for ( i = 0; i < arg_cnt; i++ )
    {
        strcpy( tmp_path, arguments[ i ] );
        rmkey = atoi( &tmp_path[ 4 ] );

        found = SDIP_FALSE;

        for ( j = 0; j < get_paths_pkt.num_output; j++ )
        {
            if  ( rmkey == rmkeys[ j ] )
            {
                found = SDIP_TRUE;
                
                path_pkt.version    = MPIO_IOCTL_VERSION_1;
                path_pkt.path_rmkey = rmkey;

                status = ioctl( devicefd, ioctl_cmd, &path_pkt );
                if  ( status != 0 )
                {
                    (void) pfmt( stderr, MM_ERROR, ioctl_err_msg,
                                 arguments[ i ], errno );
                    return_error = SDIP_TRUE;
                }   
                else
                {
                    printf( ioctl_ok_msg, arguments[ i ] );
                }
            }
        }

        /*
         *  If we did not find the user specified path in the array
         *  maintained by MPIO, display a messages and set our
         *  variable to return an error status.
         */
        if  ( !found )
        {
            (void) pfmt( stderr, MM_ERROR, MSG_NO_PATH,
                         arguments[ i ], device );
            return_error = SDIP_TRUE;
        }
    }
    
    /*
     *  Close the disk device.
     */
    close( devicefd );

    /*
     *  Determine whether or not to return an error or ok.
     */
    if  ( return_error )
    {
        return( SDIP_Exit_Error );
    }
    else
    {
        return( SDIP_Exit_Ok );
    }
    
} /* sdipath_manage() */

/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_switch()
 *
 *  Description:
 *
 *      This function manages the switch operation. Once the parameters
 *      have been validated this function opens the specified device
 *      and instructs the MPIO driver to switch the local for this
 *      device.
 *
 *  Parameters:
 *
 *      char            *device
 *                      Name of device to switch locales for.
 *
 *      int             s_locale
 *                      Source locale to switch from.
 *
 *      int             d_locale
 *                      Destination locale to switch to.
 *
 *  Return Values:
 *
 *      SDIP_Exit_Ok     Function completed without error.
 *      SDIP_Exit_Error  Function encountered an error.
 *      SDIP_Exit_Usage  Function encountered a usage error.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

int
sdipath_switch( char    *device,
                int     s_locale,
                int     d_locale )
{
    int                         status   = SDIP_Exit_Ok;
    int                         devicefd = 0;
    mpio_ioctl_switch_group_t   switch_group_pkt;
    
/*********************************************************************/

    /*
     *  Make sure there is a non NULL device specified.
     */
    if  ( device == NULL )
    {
        pfmt( stderr, MM_ERROR, MSG_NODEV, "switch" );
        status = SDIP_Exit_Usage;
    }

    /*
     *  Make sure both the source and destination local values were
     *  specified.
     */
    if  ( s_locale == SDIP_INVALID_LOCALE )
    {
        pfmt( stderr, MM_ERROR, MSG_NOLOCALE, "Source" );
        status = SDIP_Exit_Usage;
    }

    if  ( d_locale == SDIP_INVALID_LOCALE )
    {
        pfmt( stderr, MM_ERROR, MSG_NOLOCALE, "Destination" );
        status = SDIP_Exit_Usage;
    }

    /*
     *  If any of the above checks failed, return a usage error.
     */
    if  ( status != SDIP_Exit_Ok )
    {
        return( status );
    }

    /*
     *  Open the specified device.
     */
    status = sdipath_open_device( device, &devicefd );
    if  ( status == SDIP_Exit_Error )
    {
        /*
         *  Specific error message displayed from open routine, just
         *  return the error status here.
         */
        return( status );
    }

    /*
     *  Fill up a switch group ioctl packet.
     */
    switch_group_pkt.version        = MPIO_IOCTL_VERSION_1;
    switch_group_pkt.source         = s_locale;
    switch_group_pkt.destination    = d_locale;

    /*
     *  Issue the ioctl to switch locales.
     */
    status = ioctl( devicefd,
                    MPIO_IOCTL_SWITCH_GROUP,
                    &switch_group_pkt );
    
    if  ( status == SDIP_Exit_Ok )
    {
        printf( MSG_OK_SWITCH, device, d_locale );
        status = SDIP_Exit_Ok;
    }
    else
    {
        pfmt( stderr, MM_ERROR, MSG_ERR_SWITCH, device, errno );
        status = SDIP_Exit_Error;
    }

    /*
     *  Close the device and return our status.
     */
    close( devicefd );
    return( status );

} /* sdipath_switch() */

/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_reset()
 *
 *  Description:
 *
 *      This function manages the reset operation.
 *
 *  Parameters:
 *
 *      None.
 *
 *  Return Values:
 *
 *      SDIP_Exit_Ok     Function completed without error.
 *      SDIP_Exit_Error  Function encountered an error.
 *      SDIP_Exit_Usage  Function encountered a usage error.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

int
sdipath_reset()
{
    int                         status   = SDIP_Exit_Ok;
    mpio_ioctl_switch_group_t   switch_group_pkt;
    
/*********************************************************************/

    return( status );

} /* sdipath_reset() */

/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_open_device()
 *
 *  Description:
 *
 *      This function takes the user specified device and converts it
 *      to the real device name before opening it.
 *
 *  Parameters:
 *
 *      char            *device
 *                      User specified name of device to open.
 *
 *      int             *devicefd
 *                      Pointer to place to put open file descriptor.
 *
 *  Return Values:
 *
 *      SDIP_Exit_Ok     Function completed without error.
 *      SDIP_Exit_Error  Function encountered an error.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

int
sdipath_open_device( char       *device,
                     int        *devicefd )
{
    char        *real_device=NULL;
    int         status=SDIP_Exit_Ok;
    
/*********************************************************************/

    /*
     *  Find out the real device name for what we were passed.
     */
    status = sdipath_find_device( device, &real_device );
    if  ( status != SDIP_Exit_Ok )
    {
        /*
         *  Specific error message displayed from open routine, just
         *  return the error status here.
         */
        return( status );
    }

    /*
     *  Open the real device name.
     */
    *devicefd = open( real_device, O_RDWR );
    if  ( *devicefd == -1 )
    {
        pfmt( stderr, MM_ERROR, MSG_BADOPEN, real_device, errno );
        return( SDIP_Exit_Error );
    }
    
    return( status );
    
} /* sdipath_open_device() */

/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_find_device()
 *
 *  Description:
 *
 *      This function manages the search to find the real device name
 *      given what the user supplied us.
 *
 *  Parameters:
 *
 *      char            *in
 *                      Name of device to base search on.
 *
 *      char            *out
 *                      Real name of device as result of search.
 *
 *  Return Values:
 *
 *      SDIP_Exit_Ok     Function completed without error.
 *      SDIP_Exit_Error  Function encountered an error.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

int
sdipath_find_device( char       *in,
                     char       **out )
{
    char            tmp_device[ SDIP_256 ];
    int             status = SDIP_Exit_Ok;
    int             length = 0;

    char            buffer[ SDIP_64 ];
    char            **dev_list = NULL;
    char            *criteria_list[ 2 ];
    char            *cdevice;
    int             done = 0;
    int             val = 1;
    
/*********************************************************************/

    /*
     *  Store the user specified name in an array.
     */
    strcpy( tmp_device, in );

    /*
     *  First see if we were passed a device of the form cnbntndn.
     *  If we were then prepend this string with /dev/rdsk and
     *  return.
     */
    if  ( ( tmp_device[ 0 ] == 'c' ) &&
          ( tmp_device[ 2 ] == 'b' ) &&
          ( tmp_device[ 4 ] == 't' ) &&
          ( tmp_device[ 6 ] == 'd' ) )
    {
        strcpy( tmp_device, SDIP_DISK_DIR );
        strcat( tmp_device, "/" );
        strcat( tmp_device, in );

        length = strlen( tmp_device );
        if  ( tmp_device[ length - 2 ] != 'p' )
        {
            strcat( tmp_device, "p0" );
        }
        
        *out = strdup( tmp_device );
        status = SDIP_Exit_Ok;
    }

    else
    {
        /*
         *  See if we have a full path name starting with /.
         *  If so just return it.
         */
        if  ( tmp_device[ 0 ] == '/' )
        {
            *out = strdup( in );
            status = SDIP_Exit_Ok;
        }

        else
        {
            /*
             *  At this point the only thing left to check is the
             *  /etc/device.tab file for an alias. Use the getdev()
             *  call to see if the specified alias exists, if so
             *  use devattr() to get the character device to open.
             */
            sprintf( buffer, "alias=%s", in );
            criteria_list[ 0 ] = buffer;
            criteria_list[ 1 ] = NULL;

            dev_list = getdev( ( char ** ) NULL,
                               criteria_list,
                               DTAB_ANDCRITERIA );

            cdevice = devattr( *dev_list, "cdevice" );
            strcpy( tmp_device, cdevice );

            length = strlen( tmp_device );
            if  ( tmp_device[ length - 2 ] == 's' )
            {
                tmp_device[ length - 2 ] = 'p';
            }

            *out = strdup( tmp_device );
        }
    }

    return( status );
    
} /* sdipath_find_device() */

/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_error()
 *
 *  Description:
 *
 *      This is an undocumented function used to insert and remove
 *      errors in the MPIO driver. This operation can be very helpful
 *      in testing the IO redirecting of the MPIO driver.
 *
 *  Parameters:
 *
 *      char            *device
 *                      Name of device to perform operation on.
 *
 *      int             insert_error
 *                      Flag to insert error on paths.
 *
 *      int             remove_error
 *                      Flag to remove error on paths.
 *
 *      int             arg_cnt
 *                      Number of paths to operate on.
 *
 *      char            **arguments
 *                      Array of paths to operate on.
 *
 *  Return Values:
 *
 *      SDIP_Exit_Ok     Function completed without error.
 *      SDIP_Exit_Error  Function encountered an error.
 *      
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

int
sdipath_error( char     *device,
               int      insert_error,
               int      remove_error,
               int      arg_cnt,
               char     **arguments )
{
    int                 status = SDIP_Exit_Ok;
    int                 devicefd;
    int                 i, j;

    char                tmp_path[ SDIP_256 ];

    mpio_ioctl_err_t    error_pkt;
    rm_key_t            rmkey;
    
/*********************************************************************/

    /*
     *  Make sure a valid device is specified.
     */
    if  ( device == NULL )
    {
        pfmt( stderr, MM_ERROR, MSG_NODEV, "error" );
        return( SDIP_Exit_Error );
    }

    /*
     *  Make sure only one of insert or remove was specified.
     */
    if  ( ( insert_error == SDIP_FALSE ) && ( remove_error == SDIP_FALSE ) )
    {
        pfmt( stderr, MM_ERROR,
              ":16:Must specify -i or -r option for error operation.\n" );
        return( SDIP_Exit_Error );
    }
    
    /*
     *  If there were no arguments passed to this function, print
     *  an error message and return with an SDIP_Exit_Usage status.
     */
    if  ( arg_cnt == 0 )
    {
        pfmt( stderr, MM_ERROR, MSG_NOARG, "error" );
        return( SDIP_Exit_Error );
    }

    /*
     *  Open the device specified by the caller.
     */
    status = sdipath_open_device( device, &devicefd );
    if  ( status == SDIP_Exit_Error )
    {
        /*
         *  Specific error message displayed from open routine, just
         *  return the error status here.
         */
        return( status );
    }

    /*
     *  For each path that the user specified, get the device for the
     *  path name specified, then get the resource manager key for the
     *  path, fill out the packet and issue the ioctl.
     */
    for ( i = 0; i < arg_cnt; i++ )
    {
        strcpy( tmp_path, arguments[ i ] );
        rmkey = atoi( &tmp_path[ 4 ] );

        error_pkt.version      = MPIO_IOCTL_VERSION_1;
        error_pkt.path_rmkey   = rmkey;
        error_pkt.assert_error = insert_error;

        status = ioctl( devicefd, MPIO_IOCTL_INSERT_ERROR, &error_pkt );
        if  ( status != 0 )
        {
            (void) pfmt( stderr, MM_ERROR, "NOT",
                         arguments[ i ], errno );
        }   
        else
        {
            if  ( insert_error)
            {
                printf( "Error has been inserted on path %s.\n",
                        arguments[ i ] );
            }
            else
            {
                printf( "Error has been removed from path %s.\n",
                        arguments[ i ] );
            }
        }

    }
    
    /*
     *  Close the disk device.
     */
    close( devicefd );
    return( status );
    
} /* sdipath_error() */

/*
 * *******************************************************************
 *
 *  Name:
 *
 *      sdipath_usage()
 *
 *  Description:
 *
 *      This function displays the usage message and is called when
 *      there is a command syntax error.
 *
 *  Parameters:
 *
 *      char                *format
 *                          Message text to format and print.
 *
 *  Return Values:
 *
 *      None.
 *
 *  Notes:
 *
 *      None.
 *
 * *******************************************************************
 */

void
sdipath_usage(char  *format, ...)
{
    va_list         ap;
    char            buffer[ BUFSIZ ];

/*********************************************************************/

    if  ( format[0] != '\0' )
    {
        va_start( ap, format );
        vsprintf( buffer, format, ap );
        va_end( ap );

        pfmt( stderr, MM_ERROR, buffer );
    }

    pfmt( stderr,
          MM_ACTION,
          ":17:Usage: \n\
    sdipath -o list [ -qv ] [ -d device ] \n\
    sdipath -o fail -d device path ... \n\
    sdipath -o repair -d device path ... \n\
    sdipath -o switch -d device -S source-locale -D destination-locale\n" );

    exit( SDIP_Exit_Usage );

} /* sdipath_usage() */

