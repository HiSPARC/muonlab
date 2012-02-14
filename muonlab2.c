#include <rs232.h>
#include <windows.h> //voor sleep()
#include <analysis.h>
#include <ansi_c.h>
#include "asynctmr.h"
#include <formatio.h>
#include <math.h>
#include <utility.h>
#include <cvirte.h>	
#include <userint.h>
#include "muonlab.h"

#define NUM_ITEMS_IN_QUEUE_DATA 100000   //aantal plaatsen in de data queue
#define NUM_ITEMS_IN_QUEUE_FILE 50000    //aantal plaatsen in de file queue

/*---------------------------------------------------------------------------*/
/* Module Globals                                                            */
/*---------------------------------------------------------------------------*/
void HVstopper(void);
char* version = "Muonlab II Version October 2011";	//version number  
static int panelHandle;						//panel handle
static int DACpanelHandle;					//panel handle 
static int HELPpanelHandle;					//panel handle
static int tsqDataHandle = 0; 				//thread safe queue handle
static int tsqFileHandle = 0; 				//thread safe queue handle 
int threadFunctionId1 = 0; 					//thread id's 
int threadFunctionId2 = 0; 					//thread id's
int threadFunctionId3 = 0; 					//thread id's
int threadFunctionId4 = 0; 					//thread id's 
static int RS232Error;						//comm error nummer
static int threadcontroll = 0;   			//flag to indicate user is exiting 
static int GUIcontroll = 0;   				//flag to indicate user is exiting   
static int comport = 0;						//comport nummer
//static double error = 0; 					//error counter
static int meting = 1;						//meting selector
double runtime = 0;							//de runtime counter
double* fitvalue;

int zoom,offset,zoom_temp,offset_temp;
double afstand,nul,nuldev,afstanddev;
//declareer de threadsave struct
typedef struct {
   	double dataPoints[4096];
	double hitcount;
	double itellen[4096];
	int firsthit;
} BufType;
DefineThreadSafeVar(BufType, SafeBuf);

/*--------------------------------------------------------------------------*/
/* Internal function prototypes                                              */
/*---------------------------------------------------------------------------*/
static int CVICALLBACK CommData_Thread (void *functionData); 
static int CVICALLBACK ProcessData_Thread (void *functionData); 
static int CVICALLBACK GUI_Thread (void *functionData);
static int CVICALLBACK File_Thread (void *functionData);
void DisplayRS232Error (void);

//********Threads******************

// Main
int main (int argc, char *argv[])
{	  	
	double fitvaluearray[4096];
	fitvaluearray[4095]=666;
	fitvalue=&fitvaluearray[0];

	if (InitCVIRTE (0, argv, 0) == 0) {		 //initialiseer CVI
		MessagePopup ("Fatal Error", "Out of memory"); /* out of memory */
	}
	if ((panelHandle = LoadPanel (0, "muonlab.uir", PANEL)) < 0) {	  //initialiseer main panel
		MessagePopup ("Fatal Error", "Error loading MAIN panel");
	}
	if ((DACpanelHandle = LoadPanel (0, "muonlab.uir", DACPANEL)) < 0) {   //initialiseer DAC panel
		MessagePopup ("Fatal Error", "Error loading DAC panel");
	}
	if ((HELPpanelHandle = LoadPanel (0, "muonlab.uir", HELPPANEL)) < 0) {	  //initialiseer HELP panel
		MessagePopup ("Fatal Error", "Error loading HELP panel");
	}
	if (CmtNewTSQ (NUM_ITEMS_IN_QUEUE_DATA, sizeof (int), 0, &tsqDataHandle) < 0) {  //Creates a thread-safe queue for use in the program.
        MessagePopup ("Fatal Error", "Queue error");
    }
    if (CmtNewTSQ (NUM_ITEMS_IN_QUEUE_FILE, sizeof (char[80]), 0, &tsqFileHandle) < 0) {  //Creates a thread-safe queue for use in the program.
        MessagePopup ("Fatal Error", "Queue error");
    }
    InitializeSafeBuf(); //initialiseer de TS Struct
    //start de GUI thread
    CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE,GUI_Thread,0, &threadFunctionId3);	//start de GUI thread
    SetCtrlVal(HELPpanelHandle,HELPPANEL_VERSIONMSG,version); //laat het versinummer zien inde help panel
    SetSystemAttribute (ATTR_TASKBAR_BUTTON_VISIBLE, 0); //zorg dat er geen 2 taskbar buttons zijn
	DisplayPanel (panelHandle);  //Laat de main panel zien
	RunUserInterface ();			 //start de user interface
	/* Wait for the thread functions to finish executing */  /* Release thread functions */ 
	if(threadFunctionId1 != 0) {
    	CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE,threadFunctionId1,OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
    	CmtReleaseThreadPoolFunctionID (DEFAULT_THREAD_POOL_HANDLE, threadFunctionId1);
    }
    if(threadFunctionId2 != 0) {
    	CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE,threadFunctionId2,OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
    	CmtReleaseThreadPoolFunctionID (DEFAULT_THREAD_POOL_HANDLE, threadFunctionId2);
    }
    if(threadFunctionId4 != 0) {
    	CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE,threadFunctionId4,OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
    	CmtReleaseThreadPoolFunctionID (DEFAULT_THREAD_POOL_HANDLE, threadFunctionId4);
    }
    CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE,threadFunctionId3,OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
    CmtReleaseThreadPoolFunctionID (DEFAULT_THREAD_POOL_HANDLE, threadFunctionId3);  
    
	DiscardPanel (panelHandle); //gooi de main panel weg als er af is gesloten
	
	if (tsqDataHandle != 0){
        CmtDiscardTSQ (tsqDataHandle);   	// Discards the thread-safe queue used in the program.
	}
	if (tsqFileHandle != 0){
        CmtDiscardTSQ (tsqFileHandle);   // Discards the thread-safe queue used in the program.
	}
	return 0;
}

// Comm Thread
static int CVICALLBACK CommData_Thread (void *unused)
{	
	int freeSpace;
	int writeAmount;
	int data;
    while(1){
    	CmtGetTSQAttribute (tsqDataHandle, ATTR_TSQ_QUEUE_FREE_SPACE,&freeSpace );
    	if (freeSpace > 0) {
    		if(GetInQLen (comport) > 0) {
    		  	DisableBreakOnLibraryErrors ();
      				data = ComRdByte (comport);
      			EnableBreakOnLibraryErrors (); 
      			if(data < 0){
      				//error++;
      				RS232Error = data;
      				DisplayRS232Error ();
      			}
      			writeAmount = CmtWriteTSQData (tsqDataHandle, &data, 1, TSQ_INFINITE_TIMEOUT, NULL);
      			if(writeAmount <= 0){
      				//error++;
      			}
      		}
      		else {
   				Sleep(1);
   			}
      	}
      	else {
   			Sleep(1);
   		}
   		if (threadcontroll) break; //stop de thread als er op quit gedrukt wordt 	
    }
	return 0;
}

// Histogram thread
static int CVICALLBACK ProcessData_Thread (void *unused) {
	unsigned int queueWaarde = 0;
	int waardesinqueue;
	int valid = 0;
	int tel;
	double hitcounter = 0;
	int freeSpace;
	int writeAmount;
	unsigned int HB = 0;
   	unsigned int LB = 0;
   	unsigned int firsthitlb;
   	unsigned int firsthithb;
   	unsigned int waarde =0;
   	char fileline[80];
   	BufType *safeBufPtr;
	int mwaarde;
	int wlb,whb;
	while (1) {
	 	CmtGetTSQAttribute(tsqDataHandle, ATTR_TSQ_ITEMS_IN_QUEUE,&waardesinqueue );
	 	if (waardesinqueue > 0) {
	 		safeBufPtr = GetPointerToSafeBuf();
    		for(tel = 0;tel<waardesinqueue;tel++){
    			CmtReadTSQData (tsqDataHandle,&(unsigned int)queueWaarde ,1, TSQ_INFINITE_TIMEOUT, 0);
    			if(valid == 0) {
    				if ((queueWaarde & 0x80) != 0 ) {
    					HB = queueWaarde;
    					valid = 1;
    				}
    			}
				
    			else if(valid == 1) {
    				if ((queueWaarde & 0x80) == 0 ){
    					LB = queueWaarde;
    					waarde = ((HB & 0x3F) << 6 | (LB & 0x3F));
    					wlb=	 (LB & 0x3F);
						whb= (HB & 0x3F) << 6;
						//schrijf de data in de tsstruct
	 					safeBufPtr = GetPointerToSafeBuf();			   
       					ReleasePointerToSafeBuf();
    					firsthitlb = (LB & 0x40 );  
        				firsthithb = (HB & 0x40 );
						 
						if (meting==0)
						{
							if(firsthitlb > 0 && firsthithb ==0){   // PV:datapoints were written twice when firsthitlb & firsthithb >>0
        					safeBufPtr->firsthit = 2;
							mwaarde=2048-waarde; 
							 	if (mwaarde<0)
									{mwaarde=0;}
							
							safeBufPtr->dataPoints[mwaarde] += 1;
							safeBufPtr->hitcount++; 
							}
							
        				    if(firsthithb > 0 && firsthitlb ==0)
							{
        					safeBufPtr->firsthit = 1;
							mwaarde=2048+waarde;
							
							if (mwaarde>4095) {mwaarde=4095;}
							safeBufPtr->dataPoints[mwaarde] += 1; 
							safeBufPtr->hitcount++; 
							}
							
							if(firsthitlb > 0 && firsthithb > 0)
							{
							safeBufPtr->firsthit = 3;
							mwaarde=2048;
							safeBufPtr->dataPoints[mwaarde] +=1;
							safeBufPtr->hitcount++;
							
							}
							}
					
						else	{
							safeBufPtr->dataPoints[waarde] += 1;
							safeBufPtr->hitcount++; 
								}
							
               			//schrijf waarde in de file tsq
       					CmtGetTSQAttribute (tsqFileHandle, ATTR_TSQ_QUEUE_FREE_SPACE,&freeSpace );
    					if (freeSpace > 0) {
    						if(meting == 0){
								if(safeBufPtr->firsthit == 1){
								sprintf( fileline, "%s;%s;%s;%i;1;;%.3f;%d\n", "DeltaTime", DateStr (),TimeStr (),safeBufPtr->firsthit,(waarde*(12.5/24)),waarde);
								}
								
								if(safeBufPtr->firsthit == 2){
								sprintf( fileline, "%s;%s;%s;%i;2;%.3f;%d\n", "DeltaTime", DateStr (),TimeStr (),safeBufPtr->firsthit,(waarde*(12.5/24)),waarde);
								}
								else{									//don't write values if firsthit was undetermined.
								}
							}
								
							else if(meting == 1){
    							sprintf ( fileline, "%s;%s;%s;%.3f;%d\n", "LifeTime", DateStr (),TimeStr (),(waarde*6.25), waarde);
    						}
    						writeAmount = CmtWriteTSQData (tsqFileHandle, &fileline, 1, TSQ_INFINITE_TIMEOUT, NULL);
      						if(writeAmount <= 0){
      							//error++;
      						}
    					}
       					valid = 0;
    				}
    				else {
    					//error++; 
    					valid = 0;
    				}
    			}
    			else {
    				//error++ ;
    			}
    		}
    		ReleasePointerToSafeBuf(); 
    	}
    	else {
	    	Sleep(1);
	    }
		if (threadcontroll) break; //stop de thread als er op quit gedrukt wordt
   	}
    return 0;
}

//GUI Thread
static int CVICALLBACK GUI_Thread (void *unused)
{
	int waardesinqueue;
	double values;  
	BufType *safeBufPtr;
	int color;
	int plotoffset;
	double hitcount_temp;

	
	while (1) {
		
			safeBufPtr = GetPointerToSafeBuf();
			SetCtrlVal (panelHandle, PANEL_HITSNUMERIC, safeBufPtr->hitcount);
		    //SetCtrlVal (panelHandle, PANEL_ERRORNUMERIC, error);
			GetCtrlVal (panelHandle, PANEL_ZOOMNUMERIC, &zoom_temp);
		 	GetCtrlVal (panelHandle, PANEL_OFFSETNUMERIC, &offset_temp);
			GetCtrlVal (panelHandle, PANEL_NULWAARDE, &nul);
			GetCtrlVal (panelHandle, PANEL_NULSDEV, &nuldev);
			GetCtrlVal (panelHandle, PANEL_PLOTCOLORNUM, &color);
			GetCtrlVal (panelHandle, PANEL_Distance,&afstand);
			GetCtrlVal (panelHandle, PANEL_DISSDEV,&afstanddev);

			if(meting == 0){	
				if(safeBufPtr->firsthit == 0) {
					SetCtrlVal (panelHandle, PANEL_HITLED, 0);
   					SetCtrlVal (panelHandle, PANEL_HITLED2, 0);
				}
			
				if(safeBufPtr->firsthit == 1) {					  
					SetCtrlVal (panelHandle, PANEL_HITLED, 1);    
					SetCtrlAttribute (panelHandle, PANEL_HITLED2, ATTR_VISIBLE, 0);
					SetCtrlAttribute (panelHandle, PANEL_HITLED, ATTR_LABEL_TEXT, "PMT 1"); 
				}
				
				if(safeBufPtr->firsthit == 2) {
					SetCtrlVal (panelHandle, PANEL_HITLED, 0);
					SetCtrlAttribute (panelHandle, PANEL_HITLED2, ATTR_VISIBLE, 1);
					SetCtrlVal (panelHandle, PANEL_HITLED2, 1);	
					SetCtrlAttribute (panelHandle, PANEL_HITLED, ATTR_LABEL_TEXT, "PMT 2"); 
				}
				if(safeBufPtr->firsthit == 3) { //when firsthit is 3, no destinction between pm 1 and 2 was made
					SetCtrlVal (panelHandle, PANEL_HITLED, 0);
					SetCtrlAttribute (panelHandle, PANEL_HITLED2, ATTR_VISIBLE, 1);
					SetCtrlVal (panelHandle, PANEL_HITLED2, 0);		
					SetCtrlAttribute (panelHandle, PANEL_HITLED, ATTR_LABEL_TEXT, "Undecided"); 
				}
				
				if( (zoom_temp + offset_temp)*24.0/12.5 < 2048  &&  
					(offset_temp)*24.0/12.5 >= -2048) {
					zoom = zoom_temp;
					offset = offset_temp;
				}
				if (  (zoom_temp+offset_temp)*24.0/12.5 >= 2048 ) {
					if(offset_temp>1000 || offset_temp<-1000){
						offset_temp=0;
						offset=offset_temp;
					}
					else{
					offset=offset_temp; 
					}
					zoom=2048*(12.5/24)-offset; } 
				
				SetCtrlAttribute (panelHandle, PANEL_GRAPH, ATTR_XAXIS_GAIN, (12.5/24));
				SetCtrlAttribute (panelHandle, PANEL_GRAPH, ATTR_LABEL_TEXT, "Delta Time");
				SetCtrlAttribute (panelHandle, PANEL_ZOOMNUMERIC, ATTR_MAX_VALUE ,6400/3);  //PV was 2133
			//	SetCtrlAttribute (panelHandle, PANEL_ZOOMNUMERIC, ATTR_DFLT_VALUE ,100); 
			//	SetCtrlAttribute (panelHandle, PANEL_ZOOMNUMERIC, ATTR_MIN_VALUE , 0);
				SetCtrlAttribute (panelHandle, PANEL_OFFSETNUMERIC, ATTR_MAX_VALUE , 6400/3);
				SetCtrlAttribute (panelHandle, PANEL_OFFSETNUMERIC, ATTR_MIN_VALUE , -6400/3);
			//	SetCtrlAttribute (panelHandle, PANEL_OFFSETNUMERIC, ATTR_DFLT_VALUE , 6400/3); 
				
				if(safeBufPtr->hitcount == 0){   //zorg dat de y as geen 0 is
					hitcount_temp = 1;
				}
				else{
					hitcount_temp = safeBufPtr->hitcount;
				}
				
				SetCtrlAttribute (panelHandle, PANEL_FITBUTTON, ATTR_DIMMED, 1);
				SetCtrlAttribute (panelHandle, PANEL_CENTERFIT, ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_AMPLITUDE, ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_CALVALUE,ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_STANDEV, ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_Distance,ATTR_VISIBLE, 1);
				SetCtrlAttribute (panelHandle, PANEL_DISSDEV,ATTR_VISIBLE, 1);
				SetCtrlAttribute (panelHandle, PANEL_NULWAARDE,ATTR_VISIBLE, 1);
				SetCtrlAttribute (panelHandle, PANEL_NULSDEV,ATTR_VISIBLE, 1);
			   
				if (hitcount_temp > 99) // dont show plot buttons if acquired data is not satisfaying for a good plot.
					{
						SetCtrlAttribute (panelHandle, PANEL_FITBUTTON, ATTR_DIMMED, 0);
						SetCtrlAttribute (panelHandle, PANEL_CENTERFIT, ATTR_VISIBLE, 1);
						SetCtrlAttribute (panelHandle, PANEL_STANDEV, ATTR_VISIBLE, 1);
						SetCtrlAttribute (panelHandle, PANEL_AMPLITUDE,ATTR_VISIBLE, 1);
						SetCtrlAttribute (panelHandle, PANEL_CALVALUE,ATTR_VISIBLE, 1);
					}
				
			//	SetAxisScalingMode (panelHandle, PANEL_GRAPH, VAL_LEFT_YAXIS , VAL_MANUAL, 0,hitcount_temp); 
				SetAxisScalingMode (panelHandle, PANEL_GRAPH, VAL_LEFT_YAXIS , VAL_AUTOSCALE,0,0);//increment de y as op 
				SetCtrlAttribute (panelHandle,PANEL_GRAPH , ATTR_XAXIS_OFFSET, (double)offset); // -4096.0/2.0*12.5/24.0);
				plotoffset = (offset*24.0/12.5 + 2048 );
				DeleteGraphPlot (panelHandle, PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW); //clear plot
				PlotY (panelHandle, PANEL_GRAPH, safeBufPtr->dataPoints+plotoffset, zoom*24.0/12.5, VAL_DOUBLE, VAL_VERTICAL_BAR, VAL_SOLID_DIAMOND, VAL_SOLID, 1, color);
				
				if (fitvalue[4095]!=666)					 
				PlotY (panelHandle, PANEL_GRAPH, fitvalue+plotoffset, zoom*24/12.5, VAL_DOUBLE, 
					  VAL_CONNECTED_POINTS, VAL_SIMPLE_DOT, VAL_SOLID, 1, VAL_GREEN);
				
			}
			else if (meting == 1) {
				SetCtrlVal (panelHandle, PANEL_HITLED, 1);
				SetCtrlAttribute (panelHandle, PANEL_HITLED2, ATTR_VISIBLE, 0);
				
				if(offset_temp+zoom_temp <= 25600 && 
					offset_temp>=0){
					zoom = zoom_temp;
					offset = offset_temp;
			}
					if(offset_temp+zoom_temp >25600){
				zoom_temp=25600-offset_temp;
				offset_temp=offset_temp;
				zoom=zoom_temp;
				offset=offset_temp;
			}
				
				SetCtrlAttribute (panelHandle, PANEL_GRAPH, ATTR_XAXIS_GAIN, 6.25);
				SetCtrlAttribute (panelHandle, PANEL_GRAPH, ATTR_LABEL_TEXT, "Life Time");
				SetCtrlAttribute (panelHandle, PANEL_ZOOMNUMERIC, ATTR_MAX_VALUE ,25600);
				SetCtrlAttribute (panelHandle, PANEL_OFFSETNUMERIC, ATTR_MAX_VALUE , 25599);
				SetCtrlAttribute (panelHandle, PANEL_STANDEV, ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_Distance,ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_DISSDEV,ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_NULWAARDE,ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_NULSDEV,ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_CENTERFIT,ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_FITBUTTON, ATTR_DIMMED, 1);
				SetCtrlAttribute (panelHandle, PANEL_AMPLITUDE, ATTR_VISIBLE, 0);
				SetCtrlAttribute (panelHandle, PANEL_CALVALUE, ATTR_VISIBLE,0);
				SetCtrlAttribute (panelHandle, PANEL_HITLED, ATTR_LABEL_TEXT, "PMT 1"); 
				//SetCtrlAttribute (panelHandle, PANEL_CALVALUE, ATTR_LABEL_TEXT, "Damping Fit");
				
				if(safeBufPtr->hitcount == 0){   //zorg dat de y as geen 0 is
					hitcount_temp = 1;
				}
				else{
					hitcount_temp = safeBufPtr->hitcount;
				}
				if (hitcount_temp > 99)
				{
				SetCtrlAttribute (panelHandle, PANEL_CALVALUE,ATTR_VISIBLE, 1);
				SetCtrlAttribute (panelHandle, PANEL_FITBUTTON, ATTR_DIMMED, 0);
				SetCtrlAttribute (panelHandle, PANEL_STANDEV, ATTR_VISIBLE, 1);
				SetCtrlAttribute (panelHandle, PANEL_AMPLITUDE, ATTR_VISIBLE, 1);
				}
				
				SetAxisScalingMode (panelHandle, PANEL_GRAPH, VAL_LEFT_YAXIS , VAL_AUTOSCALE, 0,0); //zet de y as op 1
				SetCtrlAttribute (panelHandle,PANEL_GRAPH , ATTR_XAXIS_OFFSET, (double)offset);
				DeleteGraphPlot (panelHandle, PANEL_GRAPH, -1, VAL_IMMEDIATE_DRAW); //clear plot
				plotoffset = offset*(1.0/6.25);
				
				PlotY (panelHandle, PANEL_GRAPH, safeBufPtr->dataPoints+plotoffset, zoom*(1.0/6.25), VAL_DOUBLE,
					VAL_FAT_STEP, VAL_SOLID_DIAMOND, VAL_SOLID, 1, color);
			
		        if (fitvalue[4095]!=666)					 
				PlotY (panelHandle, PANEL_GRAPH, fitvalue+plotoffset, zoom*(1.0/6.25), VAL_DOUBLE, 
					  VAL_CONNECTED_POINTS, VAL_ASTERISK, VAL_SOLID, 1, VAL_GREEN);
    		}
							
		ReleasePointerToSafeBuf();
		Sleep(1000);
		if (GUIcontroll) break; //stop de thread als er op quit gedrukt wordt 
	}
	HVstopper();
	return 0;
}

void HVstopper(void) {
	unsigned int HB = 0;
   	unsigned int LB = 0;
	int dacwaarde=300;
	// turn off PMT1
	  	HB = 0xA << 4 | ((int)((dacwaarde-300)*0.2125) & 0xF0) >> 4 ;
        LB = 0x2 << 4 | ((int)((dacwaarde-300)*0.2125) & 0x0F);
        DisableBreakOnLibraryErrors ();
				ComWrtByte (comport, HB);
				ComWrtByte (comport, LB);
		EnableBreakOnLibraryErrors ();		
				
	// turn off PMT2
		HB = 0x9 << 4 | ((int)((dacwaarde-300)*0.2125) & 0xF0) >> 4 ;
        LB = 0x1 << 4 | ((int)((dacwaarde-300)*0.2125) & 0x0F);
            
        DisableBreakOnLibraryErrors ();
				ComWrtByte (comport, HB);
				ComWrtByte (comport, LB);
		EnableBreakOnLibraryErrors ();
	return;
}

//File Thread
static int CVICALLBACK File_Thread (void *unused)
{
	int waardesinqueue;
	char* waarde;
	int file_error;
	int hitfilehandel;
	char hitfilename[MAX_PATHNAME_LEN];
	char filewritebuf[80];
   	int filewritebufsize;
   	int close_error;
	if(meting==0)
	{file_error = FileSelectPopup ("", "Deltatime_hitdata.txt", "", "Select Destination File", VAL_SAVE_BUTTON, 0, 0, 1, 1, hitfilename); }
	else{file_error = FileSelectPopup ("", "Lifetime_hitdata.txt", "", "Select Destination File", VAL_SAVE_BUTTON, 0, 0, 1, 1, hitfilename);}
	
	if( file_error < 0 | file_error > 0){
		hitfilehandel = OpenFile (hitfilename, VAL_WRITE_ONLY, VAL_TRUNCATE, VAL_ASCII);
		if(hitfilehandel == -1){
			ConfirmPopup ("FILE ERROR", "Error opening file");
		}
		else{
			while (1) { 
				CmtGetTSQAttribute(tsqFileHandle, ATTR_TSQ_ITEMS_IN_QUEUE,&waardesinqueue );
	 			if (waardesinqueue > 0) {
	 				CmtReadTSQData (tsqFileHandle,&filewritebuf ,1, TSQ_INFINITE_TIMEOUT, 0); //haal de string uit de queue
      			  	filewritebufsize = StringLength(filewritebuf); //bepaal de string lengte
     				WriteFile (hitfilehandel, filewritebuf, filewritebufsize); //schrijf de string
     			}
     			else {
     				Sleep(1);				
     			}
				if (threadcontroll) { 
					close_error = CloseFile (hitfilehandel); //close file
					if(close_error < 0) {
						ConfirmPopup ("FILE ERROR", "Error closing file");
					}
					break; //stop de thread als er op quit gedrukt wordt 
				}
			}
		}
	}
	return 0;
}

//runtimer AsyncTimer thread
int CVICALLBACK RunTime(int reserved, int timerId, int event, void *callbackData, int eventData1, int eventData2)
{
	runtime++;
	SetCtrlVal (panelHandle, PANEL_RUNTIMENUMERIC, runtime);
	return 0;
}

//******** Main panel Callbacks *********

int CVICALLBACK StartStopButton (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int ledstat;
	int filewrite;
	int metingswitch;
	switch (event)
		{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle, PANEL_LED, &ledstat);
			if(ledstat == 0){
				SetCtrlVal (panelHandle, PANEL_LED, 1);
				SetCtrlAttribute (panelHandle, PANEL_FILESAVEBUTTON, ATTR_DIMMED, 1);
				SetCtrlAttribute (panelHandle, PANEL_COMPORTRING, ATTR_DIMMED, 1);
				SetCtrlAttribute (panelHandle, PANEL_METINGRING, ATTR_DIMMED, 1);
				SetCtrlAttribute (panelHandle, PANEL_PLOTRESETBUTTON, ATTR_DIMMED, 1);
				threadcontroll = 0;  //start de threads
				if(meting == 1){ 
					ComWrtByte (comport, 0x5F); //set lifetime
					FlushInQ (comport); 
				}
				else if(meting == 0){ 
					ComWrtByte (comport, 0x50); //set deltatime
					FlushInQ (comport);
				}
				//start de threads
				CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE,CommData_Thread,0, &threadFunctionId1);
				CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE,ProcessData_Thread,0, &threadFunctionId2);
				GetCtrlVal (panelHandle, PANEL_FILESAVEBUTTON, &filewrite);
				if(filewrite == 1) {
					CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE,File_Thread,0, &threadFunctionId4);
				}
				NewAsyncTimer (1.0, -1, 1, RunTime, 0);   //start de counter thread
			 }
			else {
				SetCtrlVal (panelHandle, PANEL_LED, 0);
				SetCtrlAttribute (panelHandle, PANEL_FILESAVEBUTTON, ATTR_DIMMED, 0);
				SetCtrlAttribute (panelHandle, PANEL_COMPORTRING, ATTR_DIMMED, 0);
			    SetCtrlAttribute (panelHandle, PANEL_METINGRING, ATTR_DIMMED, 0);
			    SetCtrlAttribute (panelHandle, PANEL_PLOTRESETBUTTON, ATTR_DIMMED, 0);
			    threadcontroll = 1;			//stop de threads
			  	DiscardAsyncTimer (-1);		//stop de counter thread
			  	runtime = 0;				//zet de runtime counter op 0
			}
			break;
		}
	return 0;
}

int CVICALLBACK ComPortRing (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle, PANEL_COMPORTRING, &comport); 
			DisableBreakOnLibraryErrors (); 
				/*  Open and Configure Com port 1 */
    			RS232Error = OpenComConfig (comport, "", 9600, 0, 8, 1, 512, 512);
    		EnableBreakOnLibraryErrors ();
    		if (RS232Error){ 
    			SetCtrlAttribute (panelHandle, PANEL_STARTSTOPBUTTON, ATTR_DIMMED, 1);
    			DisplayRS232Error ();
    		}
    		if (RS232Error == 0){ 
    			/*  Turn off Hardware handshaking (loopback test will not function with it on) */
    			SetCTSMode (comport, LWRS_HWHANDSHAKE_OFF);
    			/*  Make sure Serial buffers are empty */
   				FlushInQ (comport);
    			FlushOutQ (comport);
    			SetCtrlAttribute (panelHandle, PANEL_STARTSTOPBUTTON, ATTR_DIMMED, 0);
			}
			break;
		}
	return 0;
}

int CVICALLBACK QuitCallback (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int popupResponse;  
	switch (event)
		{
		case EVENT_COMMIT:
			popupResponse = ConfirmPopup ("Quit", " Quit Muonlab II.2 ? Set HV to zero!");
			
			if(popupResponse == 1){
				HVstopper();
				threadcontroll = 1;
				GUIcontroll = 1;
            	QuitUserInterface (0);
            }

			break;
		}
	return 0;
}

int CVICALLBACK MetingRing (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int meting_temp;
	switch (event)
		{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle, PANEL_METINGRING, &meting_temp);
			
			if(meting_temp == 0){ 
				meting=0;
				SetCtrlVal (panelHandle, PANEL_ZOOMNUMERIC, 100);
				SetCtrlVal (panelHandle, PANEL_OFFSETNUMERIC, -50);
				SetCtrlAttribute (panelHandle, PANEL_HITLED, ATTR_DIMMED, 0);
			}
			
			else if(meting_temp == 1){
				meting=1;
				SetCtrlVal (panelHandle, PANEL_ZOOMNUMERIC, 25600);
				SetCtrlVal (panelHandle, PANEL_OFFSETNUMERIC, 0); 
				SetCtrlAttribute (panelHandle, PANEL_HITLED, ATTR_DIMMED, 1);
			}
			
			break;
		}
	return 0;
}

int CVICALLBACK PlotReset (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	BufType *safeBufPtr;
	int tel ;
	switch (event)
		{
		case EVENT_COMMIT:
			//reset de data
			safeBufPtr = GetPointerToSafeBuf();
			safeBufPtr->hitcount = 0;
			safeBufPtr->firsthit = 0;
			for(tel=0;tel<4096;tel++){
   				safeBufPtr->dataPoints[tel] = 0; 
				fitvalue[tel]=0;
			}
	
			ReleasePointerToSafeBuf();
			
			SetAxisScalingMode (panelHandle, PANEL_GRAPH, VAL_LEFT_YAXIS , VAL_MANUAL, 0,1); //zet de y as op 1
					  
			break;
		}
	return 0;
}

int CVICALLBACK SavePlot (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char plotfilename[MAX_PATHNAME_LEN]; 
	static int plotbmpID = -1;
 	static int plotHandle = -1;
	switch (event)
		{
		case EVENT_COMMIT:
			FileSelectPopup ("", "plot.jpg", "", "Select Destination File", VAL_SAVE_BUTTON, 0, 0, 1, 1, plotfilename);
			GetPanelDisplayBitmap (panelHandle, VAL_VISIBLE_AREA, VAL_ENTIRE_OBJECT, &plotbmpID);
			SaveBitmapToJPEGFile (plotbmpID, plotfilename, 0, 100);
			break;
		}
	return 0;
}

int CVICALLBACK FITPlot (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	BufType *safeBufPtr;
	//general items
	double amplitude=1;
	double residue=0;
	double fittedData[4096];
	double weight[4096];
	double weight2[4096];
	double arrayX[4096];
	double arrayY[4096];
	double arrayYfit[4096];
	int i,j;
	double istart=0;
	double iend=0;
	int istartint=0;
	int iendint=0;
	double sum1=0;
	int k=0;
	double background;
	double tolerance=0.0001;
	int nrbins=0;
	AnalysisLibErrType status;
	char amplitudetext[50];

	//Gauss fit parameters
	double center=0;
	double acy=0;// initial value of center
	double standardDeviation=0.001,sigmav=0; //standev center en standev velocity
	double centernul=0; // center - nulwaarde
	double speed;
	double nulwaarde=0;   //NUL_Waarde
	double standev=0; //to calculate initial value of stan dev from center
	double hits=0;    //Sum over hits/bins, in the end equals hitcount
	double arrayI [4096];
	double arraycenter[4096];
	double arraystandev[4096];
	int istandev=istart;
	double coefs[3];
	int bm;//background midden GaussPlot
	char zerocentertext[50];
	char zerostandevtext[50];
	char centertext[50];
	char standevtext[50];
	char speedtext[50];
	char offsettext[50];
	
	//lifetime fit parameters
	double damping;
	double tau=0;
	double lifetime;
	char dampingtext[50];
	char lifetimetext[50];
	int w=0,wmax=1000;
  	int l,v,index,z;                       // index value of the likelihoods array of where it's maximum
  	double tnum[1000];
  	double tmid[1000];                     //midpoints of tnum 
  	double L[1000];                        //Array of the likelihoods
	double Errtau;                         // Analytical error
	double lnL,ArraylnL,max,sumY=0;        //To determine likelihoods and their sums and the maximum
  	double N[1000];                        //Normalisation constant 
	double taunum;
	double mtau,ftau,Errnum;               //midtau,sum of tau's and errors
	int Lvalues=0;						   // number of likelihoods values found to determine error
  
	switch (event)
		{
	case EVENT_COMMIT:
		
		if(meting ==0)
			{
			safeBufPtr = GetPointerToSafeBuf(); 

			if ((zoom_temp+offset_temp)*24.0/12.5 >= 2048 ) { 
				offset=offset_temp;
				zoom=2048*(12.5/24)-offset; }
				
				istart=2048+offset*24/12.5;	   
				iend=2048+(offset+zoom)*24/12.5;
				istartint= istart;
				iendint= iend;
				istandev=istart;

				for (i=istartint;i<iendint;i++)
				{	
					arrayY[i]=0;
					arrayI[i]=0;

					if ( safeBufPtr->dataPoints[i] != 0 )
					{
						arrayX[nrbins]=i+1;   
				    	arrayY[nrbins]=safeBufPtr->dataPoints[i];
						safeBufPtr->itellen[i]=i;
						arrayI[nrbins]=safeBufPtr->itellen[i];
						acy=(i-2048)*safeBufPtr->dataPoints[i]+acy;
						nrbins++;
					}
				}		
				
				acy=acy/safeBufPtr->hitcount; 							    //finding initial value of center.
				
//Find center waarde of i to calculate background
				bm=2048+acy+192; 			                   				//100ns left of center to calculate (assumed flat & symmetric) background. 
				for (i=bm;i<4096;i=i+1)   
				{	
					sum1=sum1+(safeBufPtr->dataPoints[i]);
				}
				background=sum1/1856;                                        // (4096-2240=1856, to calculate number of bins) 
				
				for (i=0;i<nrbins;i++)   
				{	
					arrayY[i]= arrayY[i]-background;   						 //subtract background from array
				    standev=standev+arrayY[i]*pow(((arrayI[i])-2048-acy),2); //finding initial value for standard deviation of center  
				}
				
				nrbins=k;
				center=acy+2048;                						     	 //center
				standardDeviation=sqrt(standev/safeBufPtr->hitcount);	          //RMS
				amplitude=safeBufPtr->hitcount/sqrt(6.28*pow(standardDeviation,2));// N/sqrt(2*pi*sigma^2)

				for (nrbins=0;nrbins<4096;nrbins=nrbins+1)
				{
					fitvalue [nrbins] = amplitude*exp(-pow((nrbins-center),2)/(2*pow(standardDeviation,2)));
				    if(amplitude<0)
					{ 
					fitvalue [nrbins]=0;
					}
				}
				
				fitvalue[4095]=0; // flag, was 666 
				
				center=(center-2048)/(24/12.5);
				standardDeviation=standardDeviation/sqrt(safeBufPtr->hitcount)/(24/12.5);//standev from center value
				sprintf(centertext, "%s%.2f%s", "Center:", center, " ns");
				SetCtrlVal(panelHandle, PANEL_CENTERFIT, centertext);
					   
				sprintf(amplitudetext, "%s%.2f%s", "Amplitude:", amplitude, " Hits");
				SetCtrlVal(panelHandle, PANEL_AMPLITUDE, amplitudetext);
				
				centernul= center-nul;
				centernul= fabs(centernul);
				speed=afstand/centernul;
										   
				sprintf(speedtext, "%s%.2f%s%.2f%s", "Velocity: ", speed, " cm/ns or ",speed/29.9792458, " c ");
				SetCtrlVal(panelHandle, PANEL_CALVALUE, speedtext);
				
				if(afstand==0)
					{
						sigmav=0;  //sigma of centernul
					}
			
				else{
					sigmav=sqrt(pow(standardDeviation,2)+pow(nuldev,2));  //sigma of centernul
					sigmav=sqrt(pow(sigmav/centernul,2)+pow(afstanddev/afstand,2)); //relative sigma of speed
					sigmav=sigmav*speed;
					}//sigma of speed
				
				sprintf(standevtext, "%s%.2f%s", "Standard Deviation: ", sigmav, " cm/ns");
				SetCtrlVal(panelHandle, PANEL_STANDEV, standevtext);
				
				ReleasePointerToSafeBuf(); 	  	   
		break; 
		}
			
		else
		{
			istart=offset_temp/6.25;
			iend=(offset_temp+zoom_temp)/6.25;
			istartint= istart;
			iendint= iend;
			safeBufPtr = GetPointerToSafeBuf(); 
			
			for (i=istartint;i<iendint;i=i+1)
				{	
					arrayY[i]=0;
					if ( safeBufPtr->dataPoints[i] != 0 )
					{
						arrayX[nrbins]=i+1;   
				    	arrayY[nrbins]=safeBufPtr->dataPoints[i];
						nrbins++;
					}
				}
			
			for (i=2500;i<4096;i=i+1)
				{	
					sum1=sum1+(safeBufPtr->dataPoints[i]);
				}
				
			background=(sum1/1596);     // (4096-2500=1596, to calculate number of bins) 
		  	
//Determining the Max Ln(L) and error
			ArraylnL=0;
			for(w=0;w<wmax;w++)
			{
				tnum[0]=0;    
	    		tnum[w]=w;                                   // van 0 tot 1000 bins 
	    		tmid[w]=tnum[w]+1./2.;                       //midpoints of intervals
	    		ArraylnL=0.;
	    		lnL=0;
				N[0]=0;
				N[w]=istartint/tmid[w];	                     //Log of Normalisation constant of first cut.(tmin=istartint) 
				
				
	    		for(l=0;l<nrbins;l++)
	      			{
						arrayYfit[l]=arrayY[l]-background;
						sumY=sumY+arrayYfit[l];
						lnL=arrayYfit[l]*(-log(tmid[w])-(arrayX[l]/tmid[w])+N[w]);                    //determine loglikelihood
						ArraylnL=ArraylnL+lnL;                                                        //summing loglikelihoods
	     			 } 
	    		L[w]=ArraylnL;
			}

			max=L[0];
			index=0;
			sumY=sumY/1000;

			for(v=0;v<wmax;v++)                               //determine of array the maximum value and its index
	  			{ 
  				  if(L[v]>max)
					    {
							max=L[v];
						    index=v;
		  				}
				}
			taunum=tmid[index]*6.25;
			ftau=0;
			Lvalues=0;
			
			for(z=0;z<wmax-1;z++)	                           //Calculation error of Tau
	  			{
	  				if( (L[z]-max+0.5)*(L[z+1]-max+0.5)<0.)
	    					  {
								mtau=(tmid[z]+tmid[z+1])/2.;
								ftau=ftau+mtau;
								Lvalues++;
							  }
				}
			
			Errnum=fabs(ftau/Lvalues*6.25-taunum);	
			//printf("taunum= %f with error = %f for %i \n", taunum, Errnum, Lvalues);
			
			for (nrbins=0;nrbins<4096;nrbins=nrbins+1)
				{
					  fitvalue[nrbins] = sumY/tmid[index]*exp(istart/tmid[index])*exp(-nrbins/tmid[index])+background; 
				}  
				
			fitvalue[4095]=0;                              // flag, was 666 
			//printf(" hitcount sum Y %f %f\n",safeBufPtr->hitcount, sumY);
			
			lifetime=taunum/1000;                    	   // Calculate in microseconds
			Errnum=Errnum/1000;
			
			amplitude=sumY/tmid[index]*exp(istart/tmid[index]); 
			sprintf(amplitudetext, "%s%.2f%s", "Amplitude =", amplitude, " hits"); 
			SetCtrlVal(panelHandle, PANEL_AMPLITUDE, amplitudetext);
				
			sprintf(lifetimetext, "%s%.2f%s", "Lifetime =", lifetime, " microsec");
			SetCtrlVal(panelHandle, PANEL_CALVALUE, lifetimetext);
				
			sprintf(standevtext, "%s%.2f%s", "Standard Dev.",Errnum, " microsec");
			SetCtrlVal(panelHandle, PANEL_STANDEV, standevtext);
				
			ReleasePointerToSafeBuf(); 	  
		break; 		
		}
		}
	return 0;
} 
		
int CVICALLBACK SettingsButton (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			DisplayPanel (DACpanelHandle);
			break;
		}
	return 0;
}

int CVICALLBACK HelpButton (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			DisplayPanel (HELPpanelHandle);
			break;
		}
	return 0;
}

//************* Dac panel callbacks *************

int CVICALLBACK CloseDacPanel (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			HidePanel (DACpanelHandle);
			break;
		}
	return 0;
}

int CVICALLBACK ThresholdPmt1 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double dacwaarde = 0; 
	char dactext[25];
	char HB = 0;
    char LB = 0;
	switch (event)
		{
		case EVENT_VAL_CHANGED:
			
			GetCtrlVal(DACpanelHandle,DACPANEL_THRESHOLDPMT1,&dacwaarde);
			//SetCtrlVal(DACpanelHandle,DACPANEL_NUMERICMETER1,dacwaarde);
			sprintf(dactext,"%s%.2f%s","PMT 1 Thr.: ",dacwaarde," V");
			SetCtrlVal(panelHandle,PANEL_PMT1IN,dactext);
			
			HB = 0xB << 4 | ((int)(dacwaarde*212.5) & 0xF0) >> 4 ;
            LB = 0x3 << 4 | ((int)(dacwaarde*212.5) & 0x0F);
            
            DisableBreakOnLibraryErrors ();
				ComWrtByte (comport, HB);
				ComWrtByte (comport, LB);
			EnableBreakOnLibraryErrors ();
			
			break;
		}
	return 0;
}

int CVICALLBACK ThresholdPmt2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double dacwaarde = 0;
	char dactext[25];
	char HB = 0;
    char LB = 0;
	switch (event)
		{
		case EVENT_VAL_CHANGED:
			
			GetCtrlVal(DACpanelHandle,DACPANEL_THRESHOLDPMT2,&dacwaarde);
			//SetCtrlVal(DACpanelHandle,DACPANEL_NUMERICMETER2,dacwaarde);
			sprintf(dactext,"%s%.2f%s","PMT 2 Thr.: ",dacwaarde," V");
			SetCtrlVal(panelHandle,PANEL_PMT2IN,dactext);
			
			HB = 0xC << 4 | ((int)(dacwaarde*212.5) & 0xF0) >> 4 ;
            LB = 0x4 << 4 | ((int)(dacwaarde*212.5) & 0x0F);
            
            DisableBreakOnLibraryErrors ();
				ComWrtByte (comport, HB);
				ComWrtByte (comport, LB);
			EnableBreakOnLibraryErrors ();

			break;
		}
	return 0;
}

int CVICALLBACK HVPmt1 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int dacwaarde = 0; 
	char dactext[25];
	char HB = 0;
    char LB = 0;
	switch (event)
		{
		case EVENT_VAL_CHANGED:
			
			GetCtrlVal(DACpanelHandle,DACPANEL_HVPMT1,&dacwaarde);
			//SetCtrlVal(DACpanelHandle,DACPANEL_NUMERICMETER3,dacwaarde);
			sprintf(dactext,"%s%i%s","PMT 1 HV: ",dacwaarde," V");
			SetCtrlVal(panelHandle,PANEL_PMT1HV,dactext);
			//								  printf("dacwaarde  %d ",dacwaarde);
			HB = 0xA << 4 | ((int)((dacwaarde-300)*0.2125) & 0xF0) >> 4 ;
            LB = 0x2 << 4 | ((int)((dacwaarde-300)*0.2125) & 0x0F);
            
            DisableBreakOnLibraryErrors ();
				ComWrtByte (comport, HB);
				ComWrtByte (comport, LB);
			EnableBreakOnLibraryErrors ();
            		
			break;
		}
	return 0;
}

int CVICALLBACK HVPmt2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int dacwaarde = 0;
	char dactext[25];
	char HB = 0;
    char LB = 0;
	switch (event)
		{
		case EVENT_VAL_CHANGED:
		
			GetCtrlVal(DACpanelHandle,DACPANEL_HVPMT2,&dacwaarde);
			//SetCtrlVal(DACpanelHandle,DACPANEL_NUMERICMETER4,dacwaarde);
			sprintf(dactext,"%s%i%s","PMT 2 HV: ",dacwaarde," V");
			SetCtrlVal(panelHandle,PANEL_PMT2HV,dactext);
			
			HB = 0x9 << 4 | ((int)((dacwaarde-300)*0.2125) & 0xF0) >> 4 ;
            LB = 0x1 << 4 | ((int)((dacwaarde-300)*0.2125) & 0x0F);
            
            DisableBreakOnLibraryErrors ();
				ComWrtByte (comport, HB);
				ComWrtByte (comport, LB);
			EnableBreakOnLibraryErrors ();

			break;
		}
	return 0;
}

//************* Helppanel callbacks *************   

int CVICALLBACK HelpPanelCloseButton (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			HidePanel (HELPpanelHandle);
			break;
		}
	return 0;
}

/*---------------------------------------------------------------------------*/
/* Display error information to the user.                                    */
/*---------------------------------------------------------------------------*/
void DisplayRS232Error (void)
{
    char ErrorMessage[200];
    switch (RS232Error)
        {
        default :
            if (RS232Error < 0)
                {  
                Fmt (ErrorMessage, "%s<RS232 error number %i", RS232Error);
                MessagePopup ("RS232 Message", ErrorMessage);
                }
            break;
        case 0  :
            MessagePopup ("RS232 Message", "No errors.");
            break;
        case -1  :
            MessagePopup ("RS232 Message", "Connection Lost!");
            break;
        case -2 :
        	if(comport != 0){
            	Fmt (ErrorMessage, "%s", "Invalid port (port is unavailable).");
            	MessagePopup ("RS232 Message", ErrorMessage);
            }
            break;
        case -3 :
            Fmt (ErrorMessage, "%s", "No port is open.\n"
                 "Check COM Port setting in Configure.");
            MessagePopup ("RS232 Message", ErrorMessage);
            break;
        case -99 :
            Fmt (ErrorMessage, "%s", "Timeout error.\n\n"
                 "Either increase timeout value,\n"
                 "       check COM Port setting, or\n"
                 "       check device.");
            MessagePopup ("RS232 Message", ErrorMessage);
            break;
        }
}
