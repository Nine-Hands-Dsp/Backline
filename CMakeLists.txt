cmake_minimum_required(VERSION 3.22)

# Backline (NINE HANDS | DSP), anciennement Plugin manager : application Windows / macOS qui compare les VST3 installes
# dans un dossier aux dernieres versions publiees sur GitHub, et les installe ou les met a jour.
project(PluginManager VERSION 1.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

# JUCE : dossier local si JUCE_DIR est fourni, sinon telechargement automatique
set(JUCE_DIR "" CACHE PATH "Copie locale de JUCE (optionnel)")
if(JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
  add_subdirectory(${JUCE_DIR} JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW ON)
  FetchContent_MakeAvailable(JUCE)
endif()

# Session (NINE HANDS | DSP) : moteur et fenetre d'export / import de projets REAPER, pour l'onglet Session.
# Copie locale : -DSESSION_DIR=<clone de Nine-Hands-Dsp/Session>
set(SESSION_DIR "" CACHE PATH "Copie locale du depot Session (optionnel)")
if(SESSION_DIR AND EXISTS "${SESSION_DIR}/Source/SessionCore.cpp")
  set(SESSION_SRC "${SESSION_DIR}/Source")
else()
  include(FetchContent)
  FetchContent_Declare(NhSession
    GIT_REPOSITORY https://github.com/Nine-Hands-Dsp/Session.git
    GIT_TAG f39a1958b4d5cb1d89bab1786a1a8e0b37eebf74
    SOURCE_SUBDIR "sources-only")      # sources seulement (pas l'application Session)
  FetchContent_MakeAvailable(NhSession)
  set(SESSION_SRC "${nhsession_SOURCE_DIR}/Source")
endif()

juce_add_gui_app(PluginManager
  PRODUCT_NAME "Backline"
  VERSION ${PROJECT_VERSION}
  COMPANY_NAME "NINE HANDS | DSP"
  BUNDLE_ID com.ninehandsdsp.pluginmanager
  ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png)

target_sources(PluginManager PRIVATE
  Source/Main.cpp
  Source/Platform.cpp
  ${SESSION_SRC}/SessionCore.cpp)
target_include_directories(PluginManager PRIVATE ${SESSION_SRC})

file(GLOB CATALOG_IMAGES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/catalog/img/*.jpg)
juce_add_binary_data(PMData SOURCES
  assets/fonts/Poppins-Regular.ttf
  assets/fonts/Poppins-Medium.ttf
  assets/fonts/Poppins-Bold.ttf
  assets/logo/backline_logo_white.png
  catalog/plugins.json
  ${CATALOG_IMAGES})

target_compile_definitions(PluginManager PRIVATE
  APP_VERSION="${PROJECT_VERSION}"
  JUCE_APPLICATION_NAME_STRING="Backline"
  JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(PluginManager PRIVATE
  PMData
  juce::juce_core
  juce::juce_events
  juce::juce_data_structures
  juce::juce_audio_basics
  juce::juce_audio_formats
  juce::juce_cryptography
  juce::juce_graphics
  juce::juce_gui_basics
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags)

if(WIN32)
  target_link_libraries(PluginManager PRIVATE version shell32)
endif()
