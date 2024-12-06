@ECHO OFF
SETLOCAL enabledelayedexpansion

REM Main Script

REM Capture the action argument (e.g., "Tektagon")
SET action=%1

IF /I "%action%"=="tektagon" (
    REM Collect additional arguments
    SET args=%*

    REM Define essential variables
    SET build_dir=build
    SET platform=ast1060_evb
    SET project_name=ApplicationLayer\tektagon
    SET wrk_dir=%CD%

    ECHO.
    ECHO Working Directory: !wrk_dir!
    ECHO Build Directory: !wrk_dir!\oe-src\!build_dir!
    ECHO Platform: !platform!
    ECHO Project Name: !project_name!

    REM Call the dependencies function to check or install prerequisites
    CALL :west_dependencies !wrk_dir!

    REM Check if patches have already been applied
    IF NOT EXIST oe-src\FunctionalBlocks\Cerberus\projects\zephyr (
        ECHO.
        ECHO Applying patches
        ECHO.

        REM Copy Zephyr patches
        COPY /Y zephyr_patches\Kconfig.zephyr oe-src\.
        COPY /Y zephyr_patches\CMakeLists.txt oe-src\.
        COPY /Y zephyr_patches\pfr_aspeed.h oe-src\include\drivers\misc\aspeed\
        COPY /Y zephyr_patches\pfr_ast1060.c oe-src\drivers\misc\aspeed\
        COPY /Y zephyr_patches\i2c_aspeed.c oe-src\drivers\i2c\

        REM Copy Cerberus patches
        COPY /Y cerberus_patches\Kconfig oe-src\FunctionalBlocks\Cerberus\
        COPY /Y cerberus_patches\CMakeLists.txt oe-src\FunctionalBlocks\Cerberus\
        XCOPY /E /Y /I cerberus_patches\zephyr\ oe-src\FunctionalBlocks\Cerberus\projects\zephyr
        COPY /Y cerberus_patches\i2c_slave_common.h oe-src\FunctionalBlocks\Cerberus\core\i2c\
        COPY /Y cerberus_patches\i2c_filter.h oe-src\FunctionalBlocks\Cerberus\core\i2c\
        COPY /Y cerberus_patches\debug_log.h oe-src\FunctionalBlocks\Cerberus\core\logging\
        COPY /Y cerberus_patches\rsah.patch oe-src\FunctionalBlocks\Cerberus\core\crypto
        COPY /Y cerberus_patches\logging_flash.h oe-src\FunctionalBlocks\Cerberus\core\logging\
        COPY /Y cerberus_patches\logging_flash.c oe-src\FunctionalBlocks\Cerberus\core\logging

        REM Apply patches
        git apply cerberus_patches\rsah.patch
        CD oe-src\
        ECHO Current Directory: %CD%

        REM Apply Zephyr patches
        FOR %%P IN (
            1_PATCH_CVE-2021-3510.patch
            2_PATCH_CVE-2021-3625.patch
            3_CVE-2021-3835.patch
            4_CVE-2021-3861.patch
            5_PATCH_CVE-2022-0553.patch
            6_CVE-2022-1041.patch
            7_CVE-FIX-CVE-2022-1042.patch
            8_CVE-FIX-CVE-2022-1841.patch
            9_CVE-2022-2741.patch
            10_CVE-2022-46392.patch
        ) DO git apply --reject --whitespace=fix ..\zephyr_patches\%%P

        ECHO.
        ECHO Dependencies updated
        ECHO Starting build

        REM Start the build process
        west !build_dir! -p -b !platform! !project_name!
		ECHO.
		GOTO :EOF
    ) ELSE (
        REM If dependencies already exist
        CD oe-src\
        ECHO.
        ECHO Dependencies found
        ECHO Starting build
        ECHO.

        REM Start the build process
        west !build_dir! -p -b !platform! !project_name!
		ECHO.
		GOTO :EOF
    )
)

ECHO.
ECHO Invalid or missing parameters.
ECHO Usage: build.cmd tektagon
ECHO.
REM Exit script
GOTO :EOF

:west_dependencies
REM Function to handle dependencies
SET wrk_dir=%1

REM Change to the working directory
PUSHD !wrk_dir!
IF NOT EXIST .west (
    REM Initialize West and install dependencies
	ECHO.
    ECHO Initializing West repository...
    west init -l west_init/
    west update
    west zephyr-export
    python -m pip install --user -r oe-src/scripts/requirements.txt >NUL 2>&1
    pip install pyelftools >NUL 2>&1
) ELSE (
    REM Update dependencies if already initialized
    python -m pip install --user -r oe-src/scripts/requirements.txt >NUL 2>&1
)
POPD
GOTO :EOF
