cmake_minimum_required(VERSION 3.22)

# Plugin manager (NINE HANDS | DSP) : application Windows / macOS qui compare les VST3 installes
# dans un dossier aux dernieres versions publiees sur GitHub, et les installe ou les met a jour.
project(PluginManager VERSION 1.1.0)

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

juce_add_gui_app(PluginManager
  PRODUCT_NAME "Plugin manager"
  VERSION ${PROJECT_VERSION}
  COMPANY_NAME "NINE HANDS | DSP"
  BUNDLE_ID com.ninehandsdsp.pluginmanager
  ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png)

target_sources(PluginManager PRIVATE
  Source/Main.cpp
  Source/Platform.cpp)

file(GLOB CATALOG_IMAGES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/catalog/img/*.jpg)
juce_add_binary_data(PMData SOURCES
  assets/fonts/Poppins-Regular.ttf
  assets/fonts/Poppins-Medium.ttf
  assets/fonts/Poppins-Bold.ttf
  assets/logo/logo_white.png
  catalog/plugins.json
  ${CATALOG_IMAGES})

target_compile_definitions(PluginManager PRIVATE
  APP_VERSION="${PROJECT_VERSION}"
  JUCE_APPLICATION_NAME_STRING="Plugin manager"
  JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(PluginManager PRIVATE
  PMData
  juce::juce_core
  juce::juce_events
  juce::juce_graphics
  juce::juce_gui_basics
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags)

if(WIN32)
  target_link_libraries(PluginManager PRIVATE version shell32)
  # toujours lance en administrateur (installation dans C:\Program Files\Common Files\VST3)
  if(MSVC)
    target_link_options(PluginManager PRIVATE "/MANIFESTUAC:level='requireAdministrator'")
  endif()
endif()
