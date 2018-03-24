cd .\x64\Release\
mkdir deploy
copy DirDiff.exe .\deploy\ 
cd deploy

%QTBIN%\windeployqt.exe DirDiff.exe

cd ..\..\..\