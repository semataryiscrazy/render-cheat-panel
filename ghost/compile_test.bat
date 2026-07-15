@echo off
cd /d "C:\Users\User\Downloads\remote\ghost"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /EHsc /std:c++17 /O2 /W3 /GS- /c /DCHEAT_DLL_EMBEDDED /Foghost_test.obj ghost.cpp 2>&1 | findstr /i "error"
