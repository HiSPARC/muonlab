/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2012. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  DACPANEL                        1
#define  DACPANEL_CLOSEDACPANELBUTTON    2       /* callback function: CloseDacPanel */
#define  DACPANEL_HVPMT2                 3       /* callback function: HVPmt2 */
#define  DACPANEL_HVPMT1                 4       /* callback function: HVPmt1 */
#define  DACPANEL_THRESHOLDPMT2          5       /* callback function: ThresholdPmt2 */
#define  DACPANEL_THRESHOLDPMT1          6       /* callback function: ThresholdPmt1 */
#define  DACPANEL_TEXTMSG                7
#define  DACPANEL_TEXTMSG_2              8
#define  DACPANEL_TEXTMSG_3              9
#define  DACPANEL_TEXTMSG_4              10
#define  DACPANEL_TEXTMSG_5              11
#define  DACPANEL_TEXTMSG_6              12
#define  DACPANEL_PICTURE                13
#define  DACPANEL_TEXTMSG_10             14
#define  DACPANEL_TEXTMSG_12             15
#define  DACPANEL_TEXTMSG_11             16
#define  DACPANEL_TEXTMSG_9              17

#define  HELPPANEL                       2
#define  HELPPANEL_TEXTBOX               2
#define  HELPPANEL_CLOSEBUTTON           3       /* callback function: HelpPanelCloseButton */
#define  HELPPANEL_PICTURE_2             4
#define  HELPPANEL_TEXTMSG_2             5
#define  HELPPANEL_PICTURE               6
#define  HELPPANEL_VERSIONMSG            7

#define  PANEL                           3
#define  PANEL_STARTSTOPBUTTON           2       /* callback function: StartStopButton */
#define  PANEL_COMPORTRING               3       /* callback function: ComPortRing */
#define  PANEL_QUITBUTTON                4       /* callback function: QuitCallback */
#define  PANEL_FILESAVEBUTTON            5
#define  PANEL_LED                       6
#define  PANEL_HITSNUMERIC               7
#define  PANEL_RUNTIMENUMERIC            8
#define  PANEL_SETTINGSBUTTON            9       /* callback function: SettingsButton */
#define  PANEL_ZOOMNUMERIC               10
#define  PANEL_OFFSETNUMERIC             11
#define  PANEL_SAVEBUTTON                12      /* callback function: SavePlot */
#define  PANEL_PLOTCOLORNUM              13
#define  PANEL_GRAPH                     14
#define  PANEL_HITLED                    15
#define  PANEL_TEXTMSG                   16
#define  PANEL_TEXTMSG_2                 17
#define  PANEL_PLOTRESETBUTTON           18      /* callback function: PlotReset */
#define  PANEL_METINGRING                19      /* callback function: MetingRing */
#define  PANEL_PMT1IN                    20
#define  PANEL_PMT2IN                    21
#define  PANEL_PMT1HV                    22
#define  PANEL_HELPPANELBUTTON           23      /* callback function: HelpButton */
#define  PANEL_PMT2HV                    24
#define  PANEL_FITBUTTON                 25      /* callback function: FITPlot */
#define  PANEL_CENTERFIT                 26
#define  PANEL_STANDEV                   27
#define  PANEL_AMPLITUDE                 28
#define  PANEL_CALVALUE                  29
#define  PANEL_PICTURE                   30
#define  PANEL_Distance                  31
#define  PANEL_NULWAARDE                 32
#define  PANEL_HITLED2                   33
#define  PANEL_DISSDEV                   34
#define  PANEL_NULSDEV                   35
#define  PANEL_SPLITTER_3                36
#define  PANEL_SPLITTER_2                37


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */ 

int  CVICALLBACK CloseDacPanel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ComPortRing(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK FITPlot(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK HelpButton(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK HelpPanelCloseButton(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK HVPmt1(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK HVPmt2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK MetingRing(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PlotReset(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK QuitCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SavePlot(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SettingsButton(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK StartStopButton(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ThresholdPmt1(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ThresholdPmt2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
