#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/TextF.h>
#include <Xm/PanedW.h>
#include <Xm/MessageB.h>
#include "epicsVersion.h"

#define PROBE_VERSION       1
#define PROBE_REVISION      0
#define PROBE_UPDATE_LEVEL  6
#define PROBE_VERSION_STRING "PROBE VERSION 1.0.6"

/*	
 *       System includes for CA		
 */

#include <caerr.h>
#include <cadef.h>
#include <db_access.h>
#include <stdio.h>
#include <string.h>

#include "probe.h"

extern Widget createAndPopupProductDescriptionShell();

/*
 *           X's stuffs
 */

static XmStringCharSet charset = (XmStringCharSet) XmSTRING_DEFAULT_CHARSET;
char	     fontName[] =
             "-adobe-times-bold-i-normal--18-180-75-75-p-98-iso8859-1";
XFontStruct  *font = NULL;
XmFontList   fontList = NULL;
char         fontName1[] =
             "10x20";
XFontStruct  *font1 = NULL;
XmFontList   fontList1 = NULL;
char         fontName2[] =
             "-adobe-helvetica-bold-o-normal--24-240-75-75-p-138-iso8859-1";
XFontStruct  *font2 = NULL;
XmFontList   fontList2 = NULL;

char         fontName3[] =
             "-adobe-times-bold-i-normal--20-140-100-100-p-98-iso8859-1";
char         fontName4[] =
             "10x20";
char         fontName5[] =
	     "-adobe-helvetica-bold-o-normal--34-240-100-100-p-182-iso8859-1";

Widget       toplevel, panel, display, commands, start, stop, help, quit,
             nameTag, data, status, edit, adjust, history, info, option;

atom         channel;

/*
 *      probeCa.c
 */

void    startMonitor();
void    stopMonitor();
void    getChan();
void    helpMonitor();
void    quitMonitor();
void    adjustCallback();
int     probeCATaskInit();

/*
 *      probeHistory.c
 */

void    histCallback();

/*
 *      probeInfo.c
 */

void    infoCallback();

/*  
 *      probeFormat.c
 */

void   makeHistFormatStr();
void   makeInfoFormatStr();
void   formatCallback();

/*
 *     probeDial.c
 */

void initializeDials();
void createFonts();

void updateLabelDisplay();
void updateTextDisplay();
void updateDataDisplay();
void updateStatusDisplay();
void initFormat();


void createFonts() {

  /* Create fonts */

  font = XLoadQueryFont (XtDisplay(toplevel),fontName);
  if (!font) {
     font = XLoadQueryFont (XtDisplay(toplevel),fontName3);
  } 

  if (font) {
     fontList = XmFontListCreate(font, charset);
  }else {
     printf("Unable to load font! \n");
  }


  font1 = XLoadQueryFont (XtDisplay(toplevel),fontName1);
  if (!font1) {
     font1 = XLoadQueryFont (XtDisplay(toplevel),fontName4);
  } 

  if (font1) {
     fontList1 = XmFontListCreate(font1, charset);
  } else {
    printf("Unable to load font1! \n");
  }

  font2 = XLoadQueryFont (XtDisplay(toplevel),fontName2);
  if (!font2) {
     font2 = XLoadQueryFont (XtDisplay(toplevel),fontName5);
  } 

  if (font2) {
     fontList2 = XmFontListCreate(font2, charset);
  } else {
     printf("Unable to load font2! \n");
  }

}

XtAppContext app;
Widget productDescriptionShell;

main(int argc, char *argv[])
{
  int n;
  Arg        wargs[5];

  toplevel = XtVaAppInitialize(&app, "Probe", NULL, 0, 
                          &argc, argv, NULL, NULL);

#ifdef DEBUG
  XSynchronize(XtDisplay(toplevel),TRUE);
  fprintf(stderr,"\nRunning in SYNCHRONOUS mode!!");
#endif

  createFonts();
  initChannel(&channel);

  /*
   * Create a XmPanedWindow widget to hold everything.
   */
  panel = XtVaCreateManagedWidget("panel", 
                                xmPanedWindowWidgetClass,
                                toplevel,
                                XmNsashWidth, 1,
                                XmNsashHeight, 1,
                                NULL);

  /*
   * Create a XmRowColumn widget to hold the name and value of
   *    the process variable.
   */
  display = XtCreateManagedWidget("display", 
                                   xmRowColumnWidgetClass,
                                   panel, NULL, 0);
  /*
   * An XmLabel widget shows the current process variable name.
   */ 
  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  channel.d[LABEL1].w = XtCreateManagedWidget("Channel name", 
                                xmLabelWidgetClass,
                                 display, wargs, n);
  channel.d[LABEL1].proc = updateLabelDisplay;

  /*
   * An XmLabel widget shows the current process variable value.
   */ 
  n = 0;
  if (font2) {
    XtSetArg(wargs[n],  XmNfontList, fontList2); n++;
  }
  channel.d[DISPLAY1].w = XtCreateManagedWidget("Channel value", 
                                xmLabelWidgetClass,
                                display, wargs, n);
  channel.d[DISPLAY1].proc = updateDataDisplay;

  n = 0;
  channel.d[STATUS1].w = XtCreateManagedWidget("status", 
                                xmLabelWidgetClass,
                                display, wargs, n);
  channel.d[STATUS1].proc = updateStatusDisplay;

  /*
   * An XmTextField widget for user enter the process varible name 
   *   interactively.
   */ 
  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  channel.d[TEXT1].w = XtCreateManagedWidget("edit", xmTextFieldWidgetClass,
                              panel, wargs, n);

  XtAddCallback(channel.d[TEXT1].w, XmNactivateCallback, getChan,&channel);

  /*
   * Add start, stop, help, and quit buttons and register callbacks.
   * Pass the corresponding widget to each callbacks.
   */ 
  n = 0;
  XtSetArg(wargs[n],  XmNorientation, XmHORIZONTAL); n++;
  XtSetArg(wargs[n],  XmNnumColumns, 2); n++;
  XtSetArg(wargs[n],  XmNpacking, XmPACK_COLUMN);n++;
  commands = XtCreateManagedWidget("commands", 
                                   xmRowColumnWidgetClass,
                                   panel, wargs, n);

  /* 
   *  Create 'start' button.
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  start = XtCreateManagedWidget("Start", 
                                 xmPushButtonWidgetClass,
                                 commands, wargs, n);
  XtAddCallback(start,XmNactivateCallback,startMonitor,&channel);

  /* 
   *  Create 'stop' button
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  stop = XtCreateManagedWidget("Stop", 
                               xmPushButtonWidgetClass,
                               commands, wargs, n);
  XtAddCallback(stop, XmNactivateCallback,stopMonitor,&channel);

  /* 
   *  Create 'help' button
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  help = XtCreateManagedWidget("Version", 
                               xmPushButtonWidgetClass,
                               commands, wargs, n);
  XtAddCallback(help, XmNactivateCallback,helpMonitor,&productDescriptionShell);

  /* 
   *  Create 'quit' button
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  quit = XtCreateManagedWidget("Quit", 
                               xmPushButtonWidgetClass,
                               commands, wargs, n);
  XtAddCallback(quit, XmNactivateCallback,quitMonitor,&channel);

  /* 
   *  Create 'adjust' button
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  adjust = XtCreateManagedWidget("Adjust", 
                               xmPushButtonWidgetClass,
                               commands, wargs, n);
  XtAddCallback(adjust, XmNactivateCallback,adjustCallback,&channel);

  /* 
   *  Create 'hist' button
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  history = XtCreateManagedWidget("Hist", 
                               xmPushButtonWidgetClass,
                               commands, wargs, n);
  XtAddCallback(history, XmNactivateCallback,histCallback,&channel);

  /* 
   *  Create 'F2' button
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  info = XtCreateManagedWidget("Info", 
                               xmPushButtonWidgetClass,
                               commands, wargs, n);
  XtAddCallback(info, XmNactivateCallback,infoCallback,&channel);

  /* 
   *  Create 'option' button
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  option = XtCreateManagedWidget("Format", 
                               xmPushButtonWidgetClass,
                               commands, wargs, n);
  XtAddCallback(option, XmNactivateCallback,formatCallback,&channel);


  if (probeCATaskInit()) 
    exit(1);

  if (argc == 2) {
    printf("PV = %s\n",argv[1]);
    initChan(argv[1],&channel);
  }
     

  XtRealizeWidget(toplevel);
  {
    char versionString[50];
    sprintf(versionString,"%s (%s)",PROBE_VERSION_STRING,EPICS_VERSION_STRING);
    productDescriptionShell =    
      createAndPopupProductDescriptionShell(app,toplevel,
      "Probe", fontList, NULL,
      "Probe - a channel access utility",NULL,
      versionString,
      "developed at Argonne National Laboratory, by Frederick Vong", NULL,
         -1,-1,3);
  }
  XtAppMainLoop(app);

}

