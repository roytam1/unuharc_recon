@ECHO OFF
REM MAKE02.BAT - build UHARC 0.2 test archives on a DOS machine.
REM Place next to UHARC02.EXE and run from that directory.
REM Needs: real DOS / DOSBox / Win9x DOS box (UHARC02.EXE is DOS32).
REM Makes: CORPUS test files + 02_M0..02_M3.UHA, self-tested with t.
REM Bring CORPUS + *.UHA back for decoder ground-truth testing.
REM NOTE: UHARC console output needs a real console; pipes may show
REM nothing (same for uharc_u.exe). Watch the screen for progress.
SET UHARC=UHARC02.EXE
REM --- corpus (deterministic text files) ---
MD CORPUS
COPY NUL CORPUS\EMPTY.BIN >NUL
ECHO Lorem ipsum dolor sit amet, consectetur adipiscing elit.>CORPUS\T1.TXT
ECHO Sed do eiusmod tempor incididunt ut labore et dolore magna.>>CORPUS\T1.TXT
ECHO Ut enim ad minim veniam, quis nostrud exercitation ullamco.>>CORPUS\T1.TXT
ECHO Duis aute irure dolor in reprehenderit in voluptate velit.>>CORPUS\T1.TXT
ECHO Excepteur sint occaecat cupidatat non proident sunt in culpa.>>CORPUS\T1.TXT
ECHO Deserunt mollit anim id est laborum et perspiciatis unde.>>CORPUS\T1.TXT
ECHO The quick brown fox jumps over the lazy dog near the bank.>CORPUS\T2.TXT
ECHO Pack my box with five dozen liquor jugs and a lime wedge.>>CORPUS\T2.TXT
ECHO How vexingly quick daft zebras jump by the old walnut tree.>>CORPUS\T2.TXT
ECHO Sphinx of black quartz, judge my vow for the daily prize.>>CORPUS\T2.TXT
ECHO Waltz, bad nymph, for quick jigs vex the foggy farmer.>>CORPUS\T2.TXT
ECHO Glib jocks quiz nymph to vex dwarf monks from the abbey.>>CORPUS\T2.TXT
ECHO AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA>CORPUS\REPEAT.TXT
ECHO AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA>>CORPUS\REPEAT.TXT
ECHO AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA>>CORPUS\REPEAT.TXT
ECHO AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA>>CORPUS\REPEAT.TXT
ECHO AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA>>CORPUS\REPEAT.TXT
ECHO AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA>>CORPUS\REPEAT.TXT
ECHO AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA>>CORPUS\REPEAT.TXT
ECHO AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA>>CORPUS\REPEAT.TXT
REM --- mode 0 = STORE ---
%UHARC% a -d0 -y+ -o+ -p- -m0 02_M0.UHA CORPUS\EMPTY.BIN CORPUS\T1.TXT CORPUS\T2.TXT CORPUS\REPEAT.TXT
IF ERRORLEVEL 1 GOTO FAIL
%UHARC% t 02_M0.UHA
IF ERRORLEVEL 1 GOTO FAIL
%UHARC% l 02_M0.UHA
REM --- mode 1 = FAST ---
%UHARC% a -d0 -y+ -o+ -p- -m1 02_M1.UHA CORPUS\EMPTY.BIN CORPUS\T1.TXT CORPUS\T2.TXT CORPUS\REPEAT.TXT
IF ERRORLEVEL 1 GOTO FAIL
%UHARC% t 02_M1.UHA
IF ERRORLEVEL 1 GOTO FAIL
%UHARC% l 02_M1.UHA
REM --- mode 2 = NORM ---
%UHARC% a -d0 -y+ -o+ -p- -m2 02_M2.UHA CORPUS\EMPTY.BIN CORPUS\T1.TXT CORPUS\T2.TXT CORPUS\REPEAT.TXT
IF ERRORLEVEL 1 GOTO FAIL
%UHARC% t 02_M2.UHA
IF ERRORLEVEL 1 GOTO FAIL
%UHARC% l 02_M2.UHA
REM --- mode 3 = BEST ---
%UHARC% a -d0 -y+ -o+ -p- -m3 02_M3.UHA CORPUS\EMPTY.BIN CORPUS\T1.TXT CORPUS\T2.TXT CORPUS\REPEAT.TXT
IF ERRORLEVEL 1 GOTO FAIL
%UHARC% t 02_M3.UHA
IF ERRORLEVEL 1 GOTO FAIL
%UHARC% l 02_M3.UHA
DIR 02_M*.UHA
ECHO All archives built and tested OK - bring back CORPUS + *.UHA.
GOTO END
:FAIL
ECHO *** FAILED - see messages above ***
:END
SET UHARC=
