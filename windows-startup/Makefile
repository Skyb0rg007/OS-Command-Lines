# This should be run via NMAKE in a Visual Studio Developer Command Prompt

CC = cl
CFLAGS = /W3 /D _UNICODE /D UNICODE /errorReport:prompt /volatile:iso

LINK = link
CONFLAGS = /SUBSYSTEM:WINDOWS /NODEFAULTLIB 
CONLIBS = kernel32.lib shell32.lib

parrot.exe : parrot.obj
	$(LINK) $(CONFLAGS) /ENTRY:_start $(CONLIBS) parrot.obj

parrot.obj : parrot.c
	$(CC) $(CFLAGS) /c parrot.c

clean :
	del parrot.exe parrot.obj

