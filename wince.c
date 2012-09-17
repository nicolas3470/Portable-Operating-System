/*
 * Special code for WinCE to use the void main(void) 
 * interface of NT console applications
 *
 * YOU SHOULD NOT [NEED TO] MODIFY THIS FILE.
 */

/* get the correct version of Windows NT recognised */
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <winsock.h>

#include "defs.h"
#include "interrupts_private.h"

#ifdef WINCE
void main(void); /* forward declaration of main */

/* thread that takes care of messages. */
DWORD WINAPI MainHandler(void* arg){
  /* just call main function */
  main();
  /* program doesn't terminate when main ends,
     have to explicitly kill the application
  */

  return 0; /* to supress warning */
}

/* implements an adaptor from WinMain to main. Takes
   care of messages
*/

extern HANDLE system_thread;

int WINAPI WinMain(	HINSTANCE hInstance,
			HINSTANCE hPrevInstance,
			LPTSTR    lpCmdLine,
			int       nCmdShow)
{
  int id;
  MSG msg;
  HACCEL hAccelTable;

  /* start a thread to take care of messages */
  system_thread = CreateThread(NULL, 0, MainHandler,
				NULL, 0, &id);
  kprintf("The system thread started: %x\n", system_thread);

  kprintf("Starring the message loop\n");
  hAccelTable = LoadAccelerators(hInstance, 
				 (LPCTSTR)"MINITHREADS");
  while (1) {
    if (GetMessage(&msg, NULL, 0, 0)==FALSE){
      /* WM_QUIT received, exiting */
      kprintf("WM_QUIT received, exiting.\n");
      exit(0);
    }
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
      {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
      }
  }	

}
#else
#error This file can be included only for WINCE
#endif

