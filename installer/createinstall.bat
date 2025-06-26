set qt_bin=..\qt5_release\bin
set qt_binary_create="C:\Qt\Tools\QtInstallerFramework\4.10\bin\binarycreator.exe"

REM Clean out old files
call %~dp0cleaninstall.bat

REM Copy in the documentation and examples
xcopy "%~dp0..\docs\User Guide.pdf" "%documentation%" /E /H /C /R /Q /Y
mkdir "%examples%\examples"
xcopy "%~dp0..\examples" "%examples%\examples" /E /H /C /R /Q /Y
mkdir "%examples%\airfoils"
xcopy "%~dp0..\airfoils" "%examples%\airfoils" /E /H /C /R /Q /Y

REM Copy in the executable and config file and license
set release_appdir=%~dp0..\out\build\release
xcopy "%release_appdir%\acad.exe" "%application%" /E /H /C /R /Q /Y
xcopy "%release_appdir%\config.json" "%application%" /E /H /C /R /Q /Y
xcopy "%~dp0..\docs\license.txt" "%application_meta%" /E /H /C /R /Q /Y
xcopy "%~dp0..\images\mum.ico" "%application%" /E /H /C /R /Q /Y

REM Windows deploy tool for dependencies
call cmd /C "%qt_bin%\windeployqt.exe %application%\acad.exe"

REM Create the installer
cd %~dp0
cmd /C "%qt_binary_create% --offline-only -c .\config\config.xml -p packages acad_installer.exe"
