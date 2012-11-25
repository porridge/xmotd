
#ifdef MOTIF

String fallback_resources[] = { 
  "*form.background: lightsteelblue3",
  "*text*fontList: -*-times-medium-r-*-*-*-140-*", 
  "*text*background: lightsteelblue1", 

#ifdef HAVE_HTML
  "*text.width: 680", 
  "*text.height: 500",
  "*text*borderWidth: 1", 
#else
  "*text.rows: 25", 
  "*text.columns: 80",
  "*text*borderWidth: 0", 
  "*text*wordWrap: False", 
#endif

  "*title*fontList: -*-times-bold-i-*-*-*-240-*", 
  "*title*width: 500", 
  "*title*borderWidth: 0", 
  "*title*background: lightsteelblue3", 
  "*title*labelString: Message Of The Day\n\n"

  "*logo*background: lightsteelblue3",
  "*logo*borderColor: lightsteelblue3",
  "*logo*borderWidth: 0",

  "*info*fontList: -*-times-*-r-*-*-*-140-*", 
  "*info*background: lightsteelblue1",

  "*hline.width: 680", 
  "*hline.background: black", 

  "*quit.fontList: -*-times-bold-i-*-*-*-240-*",
  "*quit.labelString: Dismiss",
  "*quit*foreground: lightcyan",
  "*quit*background: lightsteelblue3",
  NULL,
};

#else

String fallback_resources[] = { 
/*  "*shell.geometry: +20+20",*/
  "*form.background: lightsteelblue3",
  "*shapeStyle: oval",
  "*Text*font: -*-times-medium-r-*-*-*-140-*",
  "*text*background: lightsteelblue1", 

#ifdef HAVE_HTML
  "*Text.width: 680", 
  "*Text.height: 500",
  "*text*borderWidth: 1", 
#else
  "*Text.height: 500", 
  "*Text.width: 680",

  "*Text*scrollVertical:    whenNeeded",
  "*Text*scrollHorizontal:  whenNeeded",
  "*Text*autoFill:          off",
  "*Text*input: False",
#endif

  "*title.font: -*-times-bold-i-*-*-*-240-*",
  "*title.width: 500",
  "*title.borderWidth: 0",
  "*title.background: lightsteelblue3",
  "*title.label: Message Of The Day\n\n\n",
  "*logo.background: lightsteelblue3",
  "*logo.borderWidth: 0",

  "*hline.borderWidth: 0",
  "*hline.height: 2", 
  "*hline.width: 700",
  "*hline.background: black",
  "*hline.foreground: black",

  "*info.font: -*-times-bold-i-*-*-*-120-*",
  "*info.borderWidth: 0",
  "*info.background: lightsteelblue3",

  "*quit.font: -*-times-bold-r-*-*-*-240-*",
  "*quit.label: Dismiss",
  "*quit*foreground: lightcyan",
  "*quit*background: lightsteelblue3",
  NULL,
};
#endif
