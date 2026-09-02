cmake_minimum_required(VERSION 3.20)
project(terminalpolyominos
  VERSION 0.5.4
  DESCRIPTION "Lightweight full-featured polyomino stacking game for Unix terminals"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(tp_game STATIC
  src/game/Bag.cpp
  src/game/Board.cpp
  src/game/Piece.cpp
  src/game/ShapeGen.cpp
  src/game/SrsKicks.cpp
  src/game/Game.cpp)
target_include_directories(tp_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(tp_game PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(terminalpolyominos
  src/main.cpp
  src/terminal/Terminal.cpp
  src/terminal/Keys.cpp
  src/input/Input.cpp
  src/render/Renderer.cpp
  src/util/Xdg.cpp
  src/util/Settings.cpp
  src/util/HighScores.cpp
  src/app/SettingsMenu.cpp
  src/app/App.cpp)
target_include_directories(terminalpolyominos PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(terminalpolyominos PRIVATE tp_game)
target_compile_definitions(terminalpolyominos PRIVATE "TP_VERSION=\"${PROJECT_VERSION}\"")
target_compile_options(terminalpolyominos PRIVATE -Wall -Wextra -Wpedantic -Werror)

include(GNUInstallDirs)

set(TP_APP_ID "io.github.garrett_webb.terminalpolyominos")
set(TP_PACKAGING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/packaging/flatpak")
set(TP_ICON_DIR "${TP_PACKAGING_DIR}/icons")

install(TARGETS terminalpolyominos RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES
  "${TP_PACKAGING_DIR}/${TP_APP_ID}.desktop"
  "${TP_PACKAGING_DIR}/${TP_APP_ID}-terminal.desktop"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(FILES
  "${TP_PACKAGING_DIR}/${TP_APP_ID}.metainfo.xml"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

if(EXISTS "${TP_ICON_DIR}/${TP_APP_ID}.svg")
  install(FILES "${TP_ICON_DIR}/${TP_APP_ID}.svg"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
elseif(EXISTS "${TP_ICON_DIR}/${TP_APP_ID}.png")
  foreach(_size IN ITEMS 128 256 512)
    set(_icon "${TP_ICON_DIR}/${TP_APP_ID}-${_size}.png")
    if(NOT EXISTS "${_icon}")
      if(_size EQUAL 256)
        set(_icon "${TP_ICON_DIR}/${TP_APP_ID}.png")
      else()
        continue()
      endif()
    endif()
    install(FILES "${_icon}"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}x${_size}/apps"
      RENAME "${TP_APP_ID}.png")
  endforeach()
else()
  message(WARNING "No app icon in ${TP_ICON_DIR}; skipping icon install")
endif()

install(CODE [[
  set(_bindir "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin")
  file(MAKE_DIRECTORY "${_bindir}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E create_symlink terminalpolyominos tpoly
    WORKING_DIRECTORY "${_bindir}")
]])

enable_testing()
add_executable(tp_tests tests/test_game.cpp)
target_include_directories(tp_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(tp_tests PRIVATE tp_game)
target_compile_options(tp_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME tp_tests COMMAND tp_tests)

add_executable(tp_input_tests tests/test_input.cpp src/input/Input.cpp src/terminal/Keys.cpp)
target_include_directories(tp_input_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_options(tp_input_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME tp_input_tests COMMAND tp_input_tests)

add_executable(tp_settings_tests
  tests/test_settings.cpp
  src/util/Settings.cpp
  src/util/Xdg.cpp
  src/input/Input.cpp)
target_include_directories(tp_settings_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_options(tp_settings_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME tp_settings_tests COMMAND tp_settings_tests)

add_executable(tp_settings_menu_tests
  tests/test_settings_menu.cpp
  src/app/SettingsMenu.cpp
  src/util/Settings.cpp
  src/util/Xdg.cpp
  src/input/Input.cpp)
target_include_directories(tp_settings_menu_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_options(tp_settings_menu_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME tp_settings_menu_tests COMMAND tp_settings_menu_tests)

add_executable(tp_scores_tests
  tests/test_scores.cpp
  src/util/HighScores.cpp
  src/util/Xdg.cpp)
target_include_directories(tp_scores_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_options(tp_scores_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME tp_scores_tests COMMAND tp_scores_tests)

message(STATUS "terminalpolyominos ${PROJECT_VERSION} (C++${CMAKE_CXX_STANDARD})")

option(TP_BUILD_GUI "Build terminalpolyominos-gui (GTK4+VTE launcher)" OFF)
if(TP_BUILD_GUI)
  add_subdirectory(gui)
endif()
