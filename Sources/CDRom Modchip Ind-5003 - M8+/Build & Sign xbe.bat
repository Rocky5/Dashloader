@Echo off

Set "nasm=%CD%\tools\nasm.exe"
Set "xbedump=%CD%\tools\xbedump.exe"

cd src

"%nasm%" xboxapp.asm -o patcher.xbe
"%xbedump%" patcher.xbe -habibi
del /q "patcher.xbe"
rename "out.xbe" "Patcher.xbe"
move "Patcher.xbe" ..\ >NUL

timeout /t 5