/* xmotd is a  message of the day displayer for X11 and dumb-terminals
 *
 * It displays a logo in the top-left corner, a 3-line title to its
 * right and a text-widget (optionally a HTML widget) for displaying
 * the message, just below. The motd filename(s) are supplied on the
 * command-line. A single button is used to sequentailly access the
 * motd(s) and to dismiss the browser when all the messages have been
 * viewed. A label displays the time of the file being viewed and
 * (optionally) the filename. It has options for automatically popping
 * down w/o user intervention and other features which I can't be
 * bothered to type-in here and would rather have you look at the
 * man-page or http://www.ee.ryerson.ca:8080/~elf/xmotd.html
 * */

/* $Id: main.c,v 1.9 1996/10/08 17:29:14 elf Exp $ */

/*
 * Copyright 1993, 1994, 1995, 1996  Luis Fernandes <elf@ee.ryerson.ca> 
 *
 * Permission to use, copy, hack, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This application is presented as is
 * without any implied or written warranty.
 *
 * libhtmlw is copyright (C) 1993, Board of Trustees of the University
 * of Illinois. See the file libhtmlw/HTML.c for the complete text of
 * the NCSA copyright.
 */

#include <stdio.h>
#include <malloc.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <errno.h>
#include <fcntl.h>

#if defined(HPUX)

#include <ndir.h>
#define dirent  direct

#else

#include <dirent.h>

#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#ifdef MOTIF

#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

#else

#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>

#endif

#ifdef HAVE_HTML
#include <HTML.h>
#endif

#include "maindefs.h"
#include "appdefs.h"
#include "main.h"

/* default bitmap that goes in the corner*/
#include "xlogo.bm"

extern time_t motdChanged();
extern void nextMessage(Widget w, caddr_t call_data, caddr_t client_data);

/* fwd decls*/
XtActionProc reallyQuit(Widget w, XButtonEvent *ev);
void Quit(Widget w, caddr_t call_data, caddr_t client_data);
void printUsage(char *str);
int numfilesToDisplay(int argc, char **argv);
int runSilentRunDeep(int argc, char **argv, float period);
messageptr freeMessage(messageptr msglist);
messageptr newMessage(char *file);

XtAppContext app_con;
Widget topLevel;				/* the application widget*/

Widget text, quit;
Widget info;

XtIntervalId timer;             /* pop-down time-out ID */

Pixmap icon_pixmap;

app_res_t app_res;				/* application resources, in main.h */

/* list of pointers to the messages that will actually be displayed */
messageptr msgslist;

int gargc;						/* globally accessible argc */
char **gargv;					/* globally accessible argv*/

char timeStamp[256];

static int alreadyForked;		/* flag to remember if we are already
								   running in the background */

#define offset(field)   XtOffset (struct _resources *, field)

static XtResource resources[] = {
  { "always","Always",XtRInt, sizeof(int),
	  offset(always),XtRString, "0"},
  { "popdown","Popdown",XtRInt, sizeof(int),
	  offset(pto),XtRString, "0"},
  { "usedomains","UseDomains",XtRInt, sizeof(int),
	  offset(usedomains),XtRString, "0"},
  { "showfilename","ShowFilename",XtRInt, sizeof(int),
	  offset(showfilename),XtRString, "0"},
  { "paranoid","Paranoid",XtRInt, sizeof(int),
	  offset(paranoid),XtRString, "0"},
  { "bitmaplogo","BitmapLogo",XtRString, sizeof(String),
	  offset(logo),XtRString, NULL},
  { "warnfile","Warnfile",XtRString, sizeof(String),
	  offset(warnfile),XtRString, NULL},
  { "wakeup","Wakeup",XtRFloat, sizeof(float),
	  offset(periodic),XtRString, "0"},
  { "stampfile","Stampfile",XtRString, sizeof(String),
	  offset(stampfile),XtRString, TIMESTAMP},
};

static XrmOptionDescRec options[] = {
  { "-always",       "always",       XrmoptionNoArg,  "1"},
  { "-showfilename", "showfilename", XrmoptionNoArg,  "1"},
  { "-usedomains",   "usedomains",   XrmoptionNoArg,  "1"},
  { "-paranoid",     "paranoid",     XrmoptionNoArg,  "1"},
  { "-popdown",      "popdown",      XrmoptionSepArg, (caddr_t) NULL},
  { "-bitmaplogo",   "bitmaplogo",   XrmoptionSepArg, (caddr_t) NULL},
  { "-warnfile",     "warnfile",     XrmoptionSepArg, (caddr_t) NULL},
  { "-wakeup",       "wakeup",       XrmoptionSepArg, (caddr_t) NULL},
  { "-stampfile",    "stampfile",    XrmoptionSepArg, (caddr_t) NULL},

};

static char shift1Trans[]="#override\n\
        Shift<Btn1Down>,Shift<Btn1Up>: reallyquit()";

static XtActionsRec xlations[]={
        {"reallyquit", (XtActionProc) reallyQuit},
};


char *
getTimeStampName()
{
  static char buf[256];
  
  sprintf(buf, "%s/%s", getenv("HOME"), app_res.stampfile);

  if(app_res.usedomains)
	{
	  char domainame[256];

	  getdomainname(domainame, 256);
  
	  strcat(buf, "."); 
	  strcat(buf, domainame);
	}

  return(buf);
  
}

/* NOTE: Jul 17 1996: This code doesn't work accurately for some
   reason; the mod-time fo a file doesn't matchup with the time
   returned by 'date' (this is on 4.1.3_U1). This is only a problem if
   the wakeup period is something like 1 minute (which I use for
   testing) -elf*/

void
updateTimeStamp(char *motdfile)
{
  struct utimbuf ut;
  time_t now;

  now = time((time_t *)NULL);

  ut.actime = now;
  ut.modtime = now;

  if(utime(motdfile,&ut))
	{
	  extern int errno;

	  if(errno==ENOENT)			/* if the file does not (1st time), 
								   create it */
		{
		  FILE *fp;
		  
		  /* I know we're asking for trouble by not checking fp; but
             since this runs from the system Xsession file, where are
             we going to write the errors to?*/
		  
		  fp=fopen(motdfile,"w");
		  fclose(fp);

		}
	  else						/* some other problem */
		{
		  perror("xmotd");
		  exit(1);
		}
	}
  
}



XtActionProc 
reallyQuit(Widget w, XButtonEvent *ev )
{
  extern char *txtbuf;

  if(txtbuf) free(txtbuf);

  if(icon_pixmap)
	{
	  XFreePixmap(XtDisplay(topLevel), icon_pixmap);
	}

  XtDestroyApplicationContext(app_con);

/*  fprintf(stderr,"Bye.\n");*/
  exit(0);
  
}


void 
/*ARGSUSED*/
Quit(Widget w, caddr_t call_data, caddr_t client_data)
{
  extern char *txtbuf;
  
  updateTimeStamp(timeStamp);
  
  if(!app_res.periodic)
	{
	  reallyQuit((Widget)NULL, (XButtonEvent *)NULL );
	}
  else  
	{
	  extern void revisitMessagesAndDisplay();
	  
	  XUnmapWindow(XtDisplay(topLevel), XtWindow(topLevel));
	  XFlush(XtDisplay(topLevel));

	  if(!alreadyForked)
		{
/*		  fprintf(stderr,"daemonizing...\n");*/
		  if(fork()) exit(0);

		  alreadyForked=1;
		}

	  runSilentRunDeep(gargc, gargv, app_res.periodic);
	  
	  revisitMessagesAndDisplay();
		
	}

}

messageptr
freeMessage(messageptr msglist)
{
  messageptr oldmsg;
  oldmsg = msglist;
  msglist = msglist->next;
  
  free(oldmsg->file);
  free(oldmsg);
  return(msglist);

} /*freeMessage*/

messageptr
newMessage(char *file)
{
  messageptr newmsg;

  if (!(newmsg = (messageptr)malloc(sizeof(struct messagenode)))) 
	{
	  perror("xmotd");
	  exit (2);					/*problems, probably no ram HA  */
	} /*if*/

  newmsg->file=(char *)calloc(1,(strlen(file)+1)*sizeof(char));

  if(!newmsg->file) 
	{
	  perror("xmotd");
	  exit(2);
	}
 
  strcpy(newmsg->file, file);
  newmsg->next = NULL;
  return(newmsg);

} /*newMessage*/


int 
numFilesToDisplay(int argc, char **argv)
{
  register int numsg=0, i;
  struct stat motdstat;
  messageptr newmsg, currentmsg = msgslist;
  
  /* i starts at 1 since argv[0] is the program name*/
  for(i=1; i<argc; i++) 
	{
      stat(argv[i], &motdstat);
      
	  if(motdstat.st_mode & S_IFDIR) /* it's a directory */
		{
		  DIR *dir;
		  struct dirent *dp;
		  char name[BUFSIZ];

		  if ((dir = opendir(argv[i]))) 
			{
			  while (dp = readdir(dir)) 
				{
				  if (dp->d_ino == 0)
					continue;
				  if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
					continue;
				  strcpy(name, argv[i]);

				  if (name[strlen(name) - 1] != '/')
					strcat(name, "/");
				  strcat(name, dp->d_name);

				  if (access(name, 0) < 0)
					continue;

				  if(motdChanged(name, timeStamp) || app_res.always) 
					{
					  newmsg = newMessage(name);

					  if (!currentmsg) /* first in list */
						msgslist = newmsg;
					  else
						currentmsg->next = newmsg;

					  currentmsg = newmsg;
					  numsg++;
					} /*if motdchanged*/

                } /*while*/

              closedir(dir);
            } /*if opendir*/
        }
      else
        if(motdChanged(argv[i], timeStamp) || app_res.always) 
		  {
            newmsg = newMessage(argv[i]);

            if (!currentmsg)	/* first in list */
              msgslist = newmsg;
            else
              currentmsg->next = newmsg;

            currentmsg = newmsg;
            numsg++;
          }
    }
  
  if((numsg || app_res.paranoid || app_res.always) && app_res.warnfile) 
	{
      newmsg = newMessage(app_res.warnfile);
      newmsg->next = msgslist;
      msgslist = newmsg;
      numsg++;
    } /*if*/
  
  return numsg;
  
}

/* I've watched waay too many WWII sub-movies, wouldn't you say? */
int
runSilentRunDeep(int argc, char **argv, float period)
{
  unsigned slumber=(float)(period*3600.0); /* hrs->secs */

  if(slumber<60) slumber=60;	/* minimum 60 seconds */

  while(1)
	{
	  int numsg, fd;
	  
	  sleep(slumber);

	  /* Check if user is still logged-in by trying to open
         "/dev/tty". If we can't open the controlling terminal then
         the user has logged-out (W. Richard Stevens _Advanced Unix
         Programming_) and xmotd can exit. */
	  
	  if((fd=open("/dev/tty", O_RDONLY, O_RDONLY)<0))
		{
		  exit(0);
		}
	  close(fd);
	  
	  numsg=numFilesToDisplay(argc, argv);

	  if(numsg) return numsg;	/* we have files to display so we can return */
			  
	}  
		
}


main(argc, argv)
int argc;
char **argv;
{
  Display *display;
  register int i, start=0;
  int numsg;
  
  if ((argc > 1) && !(strcmp(argv[1],"-help")))
	{
	  printUsage(argv[0]);		/* and exit */
	}

  /* Test to see whether we are connected to an X display. If we
	 aren't, we proceed in text-only mode: bare-bones functionality;
	 output to stdout.  Why bare-bones, I hear you asking? Well, X
	 does all the command-line options parsing for me and I don't feel
	 like duplicating all that code. So there.*/

  if((display=XOpenDisplay((char *)NULL))==NULL) 
	{

	  if(argc<2)
		{
		  fprintf(stderr, "FATAL: Missing File.\n");
		  printUsage(argv[0]);	/* and exit */
		}
	  else
		{
		  extern void runInTextMode();
		  runInTextMode(argc, argv); /* ...and exit... */

		  fprintf(stderr,"Never gets here!\n");
		  exit(0);				/* just in case */
		  
		} 
	  
	} 
  else 
	{
	  XCloseDisplay(display);
	}
  
  /* we have to init the toolkit *before* we check the command-line
     since -geom options, etc. may be specified, in which case, the
     motd-filename is *not* the 2nd argument*/
  topLevel = XtVaAppInitialize(&app_con, "XMotd", options, 
							   XtNumber(options),
							   &argc, argv, fallback_resources, 
							   NULL);
  
  XtGetApplicationResources(topLevel, (caddr_t) &app_res,
							resources, XtNumber(resources),
							(ArgList) NULL, (Cardinal) 0);

  if(argc<2)
	{
	  fprintf(stderr,"FATAL: missing filename\n");
	  printUsage(argv[0]);	/* and exit */
	}
  

  if(app_res.paranoid && !app_res.warnfile)
	{
	  fprintf(stderr,"FATAL: specified \"-paranoid\" without \"-warnfile\"\n");
	  printUsage(argv[0]);	/* and exit */
	}

  strcpy(timeStamp, getTimeStampName());

  gargc=argc;
  gargv=argv;
  
  /* first figure out how many of the files supplied on the
     command-line we will be actually displaying; i.e. we only show
     the new ones (unless -always has been specified, in which case we
     show all of them)*/
  numsg=numFilesToDisplay(argc, argv);
 
  /* if none of the messages need to be displayed we can exit ONLY if
     we are NOT told to periodically check the motds*/
  if(!numsg)
	{

	  if(!app_res.periodic)
		{
		  XtDestroyApplicationContext(app_con);		
		  exit(0);
		}
	  else
		{

		  /* fork and go to sleep; when we wake-up, check the files
			 to see if they've been modified*/
		  if(fork()) exit(0);
		  alreadyForked=1;
		  
		  numsg=runSilentRunDeep(argc, argv, app_res.periodic);	
		  
		}
	  
	}  
  
  createWidgets(numsg);
  
  /* find the first message that is to be displayed, and display it*/
  nextMessage((Widget)NULL, (caddr_t)NULL, (caddr_t)NULL);


  XtRealizeWidget(topLevel);

  XtAppMainLoop(app_con);
}


createWidgets(int anymsg)
{
  Widget form, paned, logo, mlabel, hline;
  XtTranslations shift1TransTable;

  Arg args[8];
  int n;
  
#ifdef MOTIF
  XmString xmstr;
  Pixel fg, bg;

  form=XtVaCreateManagedWidget("form", xmFormWidgetClass,topLevel, 
							   NULL);

  XtVaGetValues(form, 
				XtNbackground, &bg,
				XtNforeground, &fg, 
				NULL);
#else

  form=XtVaCreateManagedWidget("form", formWidgetClass,topLevel, 
							   XtNresizable, True,
							   NULL);
#endif

  if(app_res.logo)
	{
#ifdef MOTIF

	  icon_pixmap=XmGetPixmap(XtScreen(topLevel), 
							  app_res.logo, 
							  fg, bg);
#else
	  unsigned int width, height;
	  
	  /* read-in user-specified bitmap*/
	  int rv=XReadBitmapFile(XtDisplay(topLevel),
							 RootWindowOfScreen(XtScreen(topLevel)),
							 app_res.logo, &width, &height, &icon_pixmap, 
							 (int *)NULL, (int*)NULL); 

	  /* I could check each return value separately for each of the
         possible error-conditions, but I'm too lazy.*/
	  if(rv!=BitmapSuccess)
		{
		  fprintf(stderr,BAD_BITMAP_MESSAGE, app_res.logo);
		  exit(-1);
		}		  
#endif

	}
  else
	{
#ifdef MOTIF

	  /* display default X bitmap compiled-in*/
	  icon_pixmap=
		XCreatePixmapFromBitmapData(XtDisplay(topLevel), 
									RootWindowOfScreen(XtScreen(topLevel)), 
									xlogo_bits,xlogo_width,xlogo_height,
									fg, bg,
									DefaultDepthOfScreen(XtScreen(topLevel)));
#else
	  /* display default X bitmap compiled-in*/
	  icon_pixmap=
		XCreateBitmapFromData(XtDisplay(topLevel),
							  RootWindowOfScreen(XtScreen(topLevel)), 
							  xlogo_bits,xlogo_width,xlogo_height);
#endif

	}
  
#ifdef MOTIF
  logo=XtVaCreateManagedWidget("logo", xmLabelWidgetClass, form, 
							   XmNleftAttachment, XmATTACH_FORM,
							   XmNtopAttachment, XmATTACH_FORM,
							   XmNlabelType, XmPIXMAP,
							   XmNlabelPixmap, icon_pixmap,
							   XmNborderWidth, 0,
							   NULL);

  mlabel=XtVaCreateManagedWidget("title", xmLabelWidgetClass, form, 
								 XmNleftAttachment, XmATTACH_WIDGET,
 								 XmNleftWidget, logo,
								 XmNrightAttachment, XmATTACH_FORM,
								 NULL);

  hline=XtVaCreateManagedWidget("hline", xmSeparatorWidgetClass, form, 
								 XmNtopAttachment, XmATTACH_WIDGET,
								 XmNtopWidget, logo,
								 NULL);

  quit = XtVaCreateManagedWidget("quit", xmPushButtonWidgetClass, form, 
								 XmNtopAttachment, XmATTACH_WIDGET,
								 XmNtopWidget, logo,
								 XmNlabelType, XmSTRING,
								  NULL);

  info=XtVaCreateManagedWidget("info", xmLabelWidgetClass, form, 
							   XmNleftAttachment, XmATTACH_FORM,
							   XmNtopAttachment, XmATTACH_WIDGET,
							   XmNtopWidget, quit,
							   XmNlabelType, XmSTRING,
							   NULL);

#ifdef HAVE_HTML

   n = 0;
  XtSetArg(args[n],XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNtopWidget, info); n++;
  XtSetArg(args[n],XmNbottomAttachment, XmATTACH_FORM); n++;

  text = XtVaCreateManagedWidget("text", htmlWidgetClass, form,NULL);

  XtSetValues(text,args,n);

  XtManageChild(text);

#else

  n=0;
  XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
  XtSetArg(args[n], XmNeditable, False); n++;
  XtSetArg(args[n], XmNscrollHorizontal, False); n++;
  XtSetArg(args[n], XmNscrollLeftSide, True); n++;
  XtSetArg(args[n], XmNcursorPositionVisible, False); n++;

  text=XmCreateScrolledText(form, "text", args, n);
  n=0;
  
  XtVaSetValues(XtParent(text), 
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, info,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
  
  XtManageChild(text);
#endif

#else

  logo=XtVaCreateManagedWidget("logo", labelWidgetClass, form, 
								XtNleft, XtChainLeft,
								XtNright, XtChainLeft,
								XtNbitmap, icon_pixmap,
								NULL);
  
  mlabel=XtVaCreateManagedWidget("title", labelWidgetClass, form, 
								 XtNfromHoriz, logo,
								 NULL);

  hline=XtVaCreateManagedWidget("hline", labelWidgetClass, form, 
								 XtNfromVert, logo,
								 XtNfromVert, mlabel,
								 XtNlabel, (char *)NULL,
								 NULL);

  quit = XtVaCreateManagedWidget("quit", commandWidgetClass, form, 
								  XtNleft, XtChainLeft,
								  XtNright, XtChainLeft,
								  XtNfromVert, logo,
								  NULL);

  info=XtVaCreateManagedWidget("info", labelWidgetClass, form, 
							   XtNright, XtChainLeft,
							   XtNfromVert, quit,
							   XtNresizable, True,
							   NULL);

#ifdef HAVE_HTML
  text = XtVaCreateManagedWidget("text",
								 htmlWidgetClass, form,
								 XtNfromVert, info,
								 XtNwidth, 680,
								 XtNheight, 500,
								 NULL);

#else
  text = XtVaCreateManagedWidget("text",
								 asciiTextWidgetClass, form, 
								 XtNfromVert, info,
								 NULL);

#endif
	
#endif
	
  if((anymsg>1))
	{
#ifdef MOTIF 
	  xmstr=XmStringCreateLocalized(NEXT_MESSAGE_LABEL);
	  XtVaSetValues(quit, XmNlabelString, xmstr, NULL);
	  XmStringFree(xmstr);

	  XtAddCallback(quit, XmNactivateCallback, (XtCallbackProc)nextMessage, 0);
#else
	  XtAddCallback(quit, XtNcallback, (XtCallbackProc)nextMessage,(caddr_t)0);
	  XtVaSetValues(quit, XtNlabel, NEXT_MESSAGE_LABEL, NULL);
#endif
  	  if(app_res.pto)
	    timer=XtAppAddTimeOut(app_con, (unsigned long)(app_res.pto*1000),
			(XtTimerCallbackProc)nextMessage, (caddr_t) NULL);
	}
  else
	{
#ifdef MOTIF 
	  xmstr=XmStringCreateLocalized(LAST_MESSAGE_LABEL);
	  XtVaSetValues(quit, XmNlabelString, xmstr, NULL);
	  XmStringFree(xmstr);

	  XtAddCallback(quit, XmNactivateCallback, (XtCallbackProc)Quit, 0);
#else
	  XtAddCallback(quit, XtNcallback, (XtCallbackProc)Quit, 0);
	  XtVaSetValues(quit, XtNlabel, LAST_MESSAGE_LABEL, NULL);
#endif
  	  if(app_res.pto)
	    timer=XtAppAddTimeOut(app_con, (unsigned long)(app_res.pto*1000),
			(XtTimerCallbackProc)Quit, (caddr_t) NULL);
	}
  
  XtAppAddActions(app_con, xlations, XtNumber(xlations));
  shift1TransTable=XtParseTranslationTable(shift1Trans);
  XtOverrideTranslations(quit, shift1TransTable);
  
}/* createWidgets*/

