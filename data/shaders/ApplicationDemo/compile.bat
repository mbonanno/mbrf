FOR %%i IN (*.vert *.tesc *.tese *.geom *.frag *.comp) DO (
echo compiling %%i
..\glslc.exe %%i -o %%i.spv
)

pause
