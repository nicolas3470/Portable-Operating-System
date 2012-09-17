# Makefile for minithreads on x86/NT

# You probably only need to modify the MAIN variable to build the desired file

CC = cl.exe
LINK = link.exe

WINSOCKLIB = ws2_32.lib

###############################################################################
############################    STOP     ######################################
###############################################################################
##  If you feel it is necessary to change these variables for your platform, ##
##  please email cs4410staff@systems.cs.cornell.edu and describe your to us  ##
##  your platform, and what about these settings makes compilation fail      ##
###############################################################################

WIN32_SDKLIBPATH = "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib"
WIN32_VCLIBPATH = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib"
WIN32_WINVER = "WIN32"
WIN32_MACHINE = "I386"
WIN32_PRIMITIVES = machineprimitives_x86_asm

WOW_SDKLIBPATH = "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib"
WOW_VCLIBPATH = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib"
WOW_WINVER = "WIN32"
WOW_MACHINE = "I386"
WOW_PRIMITIVES = machineprimitives_x86_asm

X64_SDKLIBPATH = "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64"
X64_VCLIBPATH = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib\amd64"
X64_WINVER = "WIN64"
X64_MACHINE = "X64"
X64_PRIMITIVES = machineprimitives_x86_64_asm

!IF "$(PROCESSOR_ARCHITECTURE)" == "AMD64"
SDKLIBPATH = $(X64_SDKLIBPATH)
VCLIBPATH = $(X64_VCLIBPATH)
WINVER = $(X64_WINVER)
MACHINE = $(X64_MACHINE)
PRIMITIVES = $(X64_PRIMITIVES)
!ELSEIF "$(PROCESSOR_ARCHITECTURE)" == "x86"
!IF DEFINED(ProgramW6432)
SDKLIBPATH = $(WOW_SDKLIBPATH)
VCLIBPATH = $(WOW_VCLIBPATH)
WINVER = $(WOW_WINVER)
MACHINE = $(WOW_MACHINE)
PRIMITIVES = $(WOW_PRIMITIVES)
!ELSE
SDKLIBPATH = $(WIN32_SDKLIBPATH)
VCLIBPATH = $(WIN32_VCLIBPATH)
WINVER = $(WIN32_WINVER)
MACHINE = $(WIN32_MACHINE)
PRIMITIVES = $(WIN32_PRIMITIVES)
!ENDIF
!ENDIF

###############################################################################

CFLAGS = /nologo /MTd /W3 /Gm /EHsc /Zi /Od /D $(WINVER) /D "_DEBUG" /D "_CRT_SECURE_NO_WARNINGS" /D "_CONSOLE" /D "_MBCS" /Fp"minithreads.pch" /Fo"" /Fd"" /FD /RTC1 /c 
LFLAGS = /nologo /subsystem:console /incremental:no /pdb:"minithreads.pdb" /debug /machine:$(MACHINE) /out:"minithreads.exe" /LIBPATH:$(SDKLIBPATH) /LIBPATH:$(VCLIBPATH)
ASFLAGS = /c /Zi /Zf

LIB = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(WINSOCKLIB)

# change this to the name of the file you want to link with minithreads, 
# dropping the ".c": so to use "sieve.c", change to "MAIN = sieve".
#
# Note that you must write your own test programs.

MAIN = alarmtest1

SYSTEMOBJ = interrupts.obj \

OBJ = alarm.obj\
	disk.obj\
	minifile.obj\
	miniroute.obj\
	miniheader.obj\
	minisocket.obj\
	minimsg.obj\
	network.obj\
	random.obj\
	minithread.obj \
	machineprimitives_x86.obj \
	$(PRIMITIVES).obj \
	machineprimitives.obj \
	queue.obj \
	$(MAIN).obj \
	synch.obj \
	read.obj \
	multilevel_queue.obj \
	pid_random.obj

all: minithreads.exe

wince: 
	nmake /f Makefile_WinCE MAIN=$(MAIN)

.c.obj:
	$(CC) $(CFLAGS) $<

machineprimitives_x86_asm.obj: machineprimitives_x86_asm.S
	ml.exe $(ASFLAGS) machineprimitives_x86_asm.S

machineprimitives_x86_64_asm.obj: machineprimitives_x86_64_asm.S
	ml64.exe $(ASFLAGS) machineprimitives_x86_64_asm.S

minithreads.exe: start.obj end.obj $(OBJ) $(SYSTEMOBJ)
	$(LINK) $(LFLAGS) $(LIB) $(SYSTEMOBJ) start.obj $(OBJ) end.obj $(LFLAGS)

clean:
	-@del /F /Q *.obj
	-@del /F /Q minithreads.pch minithreads.pdb 
	-@del /F /Q minithreads.exe

#depend: 
#	gcc -MM *.c 2>/dev/null | sed -e "s/\.o/.obj/" > depend


include Depend
