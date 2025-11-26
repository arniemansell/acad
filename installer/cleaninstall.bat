REM Remove Stale Data
set application=%~dp0packages\application\data
set application_meta=%~dp0packages\application\meta
set examples=%~dp0packages\examples\data\examples
set airfoils=%~dp0packages\airfoils\data\airfoils
set documentation=%~dp0packages\documentation\data
set uninstall=%~dp0packages\uninstall\data

del "%application%\*.*" /s /F /Q
del "%application_meta%\license.txt" /s /F /Q
rmdir "%application%" /s /q
mkdir "%application%"

del "%airfoils%\*.*" /s /F /Q
rmdir "%airfoils%" /s /q
mkdir "%airfoils%"

del "%examples%\*.*" /s /F /Q
rmdir "%examples%" /s /q
mkdir "%examples%"

del "%documentation%\*.*" /s /F /Q
rmdir "%documentation%" /s /q
mkdir "%documentation%"

del "%uninstall%\*.*" /s /F /Q
rmdir "%uninstall%" /s /q
mkdir "%uninstall%"
