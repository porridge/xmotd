/*
 * Copyright 1993-97  Luis Fernandes <elf@ee.ryerson.ca> 
 *
 * Permission to use, copy, hack, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This application is presented as is
 * without any implied or written warranty.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * libhtmlw is copyright (C) 1993, Board of Trustees of the University
 * of Illinois. See the file libhtmlw/HTML.c for the complete text of
 * the NCSA copyright.
 */

/* $Id: logo.c,v 1.2 1997/07/18 01:23:55 elf Exp $ */
#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#ifdef HAVE_XPM
#include <X11/xpm.h>

/* default pixmap that goes in the corner*/
#include "xlogo.xpm"
#endif

#include "maindefs.h"

/* default bitmap that goes in the corner*/
#include "xlogo.bm"


/* Creates an icon pixmap and returns it in icon_pixmap. If xpm is
   supported, use it.*/
void
loadLogo(char *logo, Pixmap *icon_pixmap, Pixel fg, Pixel bg)
{
  extern Widget topLevel;
#ifdef HAVE_XPM
  Pixmap shape_mask_return;
#endif

  if(logo)
	{
#ifdef MOTIF

#ifdef HAVE_XPM
	  int rv=XpmReadFileToPixmap(XtDisplay(topLevel)
						  ,RootWindowOfScreen(XtScreen(topLevel))
						  ,logo, icon_pixmap
						  ,&shape_mask_return, NULL);
	  if(rv!=BitmapSuccess)
		{
		  fprintf(stderr,BAD_BITMAP_MESSAGE, logo);
		  exit(-1);
		}
#else

	*icon_pixmap=XmGetPixmap(XtScreen(topLevel), 
							 logo, fg, bg);
#endif /* HAVE_XPM*/

#else
	unsigned int width, height;

	/* read-in user-specified bitmap*/
	int rv=XReadBitmapFile(XtDisplay(topLevel),
						   RootWindowOfScreen(XtScreen(topLevel)),
						   logo, &width, &height, icon_pixmap, 
						   (int *)NULL, (int*)NULL); 

	if(rv!=BitmapSuccess)
	  {
		/* if xpm support is compiled in...*/
#ifdef HAVE_XPM

		/*... attempt to load it as an xpm file*/
		rv=XpmReadFileToPixmap(XtDisplay(topLevel)
							   ,RootWindowOfScreen(XtScreen(topLevel))
							   ,logo, icon_pixmap
							   ,&shape_mask_return, NULL);
#endif
		if(rv!=BitmapSuccess)
		  {
			fprintf(stderr,BAD_BITMAP_MESSAGE, logo);
			exit(-1);
		  }
	  }
#endif /*MOTIF*/
	}
  else
	{

#ifdef HAVE_XPM
	  /* display default X pixmap compiled-in*/
	  XpmCreatePixmapFromData(XtDisplay(topLevel), 
							  RootWindowOfScreen(XtScreen(topLevel)),
							  xlogo_xpm, icon_pixmap,
							  &shape_mask_return, NULL);
#else

		/* display default X bitmap compiled-in*/
	  /* Note: XCreateBitmapFromData doesn't seem to work under Motif
         for some reason, but XCreatePixmapFromBitmapData works for
         both, so we use it.*/
	  *icon_pixmap=
		XCreatePixmapFromBitmapData(XtDisplay(topLevel), 
									RootWindowOfScreen(XtScreen(topLevel)), 
									xlogo_bits,xlogo_width,xlogo_height,
									fg, bg,
									DefaultDepthOfScreen(XtScreen(topLevel)));
#if 0

		*icon_pixmap=
		  XCreateBitmapFromData(XtDisplay(topLevel),
								RootWindowOfScreen(XtScreen(topLevel)), 
								xlogo_bits,xlogo_width,xlogo_height);
#endif
#endif /*HAVE_XPM*/
		
	}
}/*loadLogo*/

  
