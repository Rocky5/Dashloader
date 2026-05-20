@Echo off & TITLE 
goto getadminwrites >NUL

:START
CD "%~dp0"
SET "Title=Dashlauncher Bios Loader"
SET "DEST=Bios Loader"
COLOR 1B
TITLE %Title%
CLS

IF "%VS71COMNTOOLS%"=="" (
  SET NET="%ProgramFiles%\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv.com"
) ELSE (
  SET NET="%VS71COMNTOOLS%\..\IDE\devenv.com"
)

IF NOT EXIST %NET% (
  CALL:ERROR "Visual Studio .NET 2003 was not found."
  GOTO:EOF
)

SET XBE_PATCH="tools\xbepatch.exe"
SET Habibi="tools\xbedump.exe"
SET XBE=default.xbe
Del "%DEST%\%XBE%" /Q 2>NUL
MKDIR "%DEST%"

ECHO Wait while preparing the build.
ECHO ------------------------------------------------------------
ECHO %NET% "dashloaderBios.sln" /build Release
%NET% "dashloaderBios.sln" /build Release

copy "Release\%XBE%" "%DEST%\%XBE%"
rmdir /S /Q "Release"
echo:
ECHO - XBE Patching %DEST%\%XBE%
%XBE_PATCH% "%DEST%\%XBE%"
ECHO - Patching Done!
(
%Habibi% "%DEST%\%XBE%" -habibi
del /q "%DEST%\%XBE%"
ren "out.xbe" "%XBE%"
move "%XBE%" "%DEST%"
)>NUL
ECHO - XBE Signing %DEST%\%XBE%
Echo - XBE Signed!
timeout /t 2
exit

:GETADMINWRITES
	REM    --> Check FOR permissions
	IF "%PROCESSOR_ARCHITECTURE%" EQU "amd64" (
		>nul 2>&1 "%SYSTEMROOT%\SysWOW64\cacls.exe" "%SYSTEMROOT%\SysWOW64\config\system"
	) ELSE (
		>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
	)
	REM --> IF error flag set, we do not have admin.
	IF '%errorlevel%' NEQ '0' (
		mode con: cols=60 lines=10
		CLS
		ECHO.
		ECHO ============================================================
		ECHO.
		ECHO             Requesting Administrative Privileges
		ECHO.
		ECHO ============================================================
		ECHO.
		GOTO UACPROMPT
	) ELSE ( GOTO GOTADMIN )

:UACPROMPT
	ECHO SET UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
	SET params = %*:"=""
	ECHO UAC.ShellExecute "cmd.exe", "/c ""%~s0"" %params%", "", "runas", 1 >> "%temp%\getadmin.vbs"
	"%temp%\getadmin.vbs"
	del "%temp%\getadmin.vbs"
	exit /B

:GOTADMIN
	pushd "%CD%"
	CD /D "%~dp0"
	GOTO START