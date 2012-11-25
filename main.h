/* $Id: main.h,v 1.5 1999/11/23 23:01:14 elf Exp $ */
typedef struct _resources {
  int always;					/* flag; if set, ignore .xmotd timestamp */
  int pto;						/* popdown time-out value in seconds*/
  int usedomains;				/* do timestamping with .xmotd.domain-name */
  int showfilename;				/* display the name of the file being 
								   viewed next to the date & time */ 
  int paranoid;					/* used with -warnfile; always display 
								   the warning message */ 

  float periodic;				/* if set, xmotd will periodically
								   check the motd files to see if they
								   have changed, and display them
								   accordingly. The value indicates
								   the sleep period in hours (decimals
								   represent minutes)*/

  int tail;						/* flag; if scroll text widget to end */

  String warnfile;				/* path to a filename containing a
								   standard warning message that is
								   displayed whenever a motd is displayed*/

  String logo;					/* path to logo (xbm) */

  String stampfile;				/* name of the timestamp filename */

  String atomname;				/* we can force multiple xmotds to run
								   by giving them unique names */
  
#ifdef HAVE_HTML
  String browser;				/* path to web-browser */
#endif

} app_res_t;

typedef struct messagenode {   /* linked list of messages to display */
  char *file;
  struct messagenode *next;
} *messageptr;
