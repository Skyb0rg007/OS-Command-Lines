
!IF !defined(WINDOWSSDKVERSION)
!ERROR Missing WindowsSDKVersion environment variable: you should be in Visual Studio Developer Command Prompt / PowerShell
!ENDIF

CC = cl
CFLAGS = /nologo /W3 /D _UNICODE /D UNICODE /errorReport:prompt /volatile:iso

LINK = link
CONFLAGS = /NOLOGO /SUBSYSTEM:WINDOWS /NODEFAULTLIB 
CONLIBS = kernel32.lib shell32.lib

RM = del

parrot.exe: parrot.obj
	$(LINK) $(CONFLAGS) /ENTRY:_start $(CONLIBS) parrot.obj

parrot.obj: parrot.c
	$(CC) $(CFLAGS) /c /Fo: $@ parrot.c

clean:
	$(RM) parrot.exe parrot.obj

