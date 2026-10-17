cmake_minimum_required(VERSION 3.20)
project(terminalpolyominos
  VERSION 0.5.6
  DESCRIPTION "Lightweight full-featured polyomino stacking game for Unix terminals"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(tp_game STATIC
  src/game/Bag.cpp
  src/game/BattleAttack.cpp
  src/game/Board.cpp
  src/game/Piece.cpp
  src/game/ShapeGen.cpp
  src/game/SrsKicks.cpp
  src/game/Game.cpp)
target_include_directories(tp_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(tp_game PRIVATE -Wall -Wextra -Wpedantic -Werror)

option(TP_ENABLE_BATTLE "TLS mediator transport (OpenSSL / Apple TLS)" OFF)
option(TP_BATTLE_INSECURE "Plain TCP battle transport (dev only)" OFF)

add_library(tp_battle STATIC
  src/battle/OppBoardView.cpp
  src/battle/LocalMediatorTransport.cpp
  src/battle/BattleSession.cpp
  src/battle/ProtocolCodec.cpp)
target_include_directories(tp_battle PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(tp_battle PUBLIC tp_game)
target_compile_options(tp_battle PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(TP_BATTLE_INSECURE)
  target_sources(tp_battle PRIVATE src/net/InsecureMediatorTransport.cpp)
  target_compile_definitions(tp_battle PUBLIC TP_BATTLE_INSECURE=1)
endif()

if(TP_ENABLE_BATTLE)
  message(FATAL_ERROR "TP_ENABLE_BATTLE (TLS transport) is not implemented yet; use -DTP_BATTLE_INSECURE=ON for dogfood")
endif()

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
  src/app/BattleMenu.cpp
  src/app/App.cpp)
target_include_directories(terminalpolyominos PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(terminalpolyominos PRIVATE tp_game tp_battle)
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

add_executable(tp_battle_tests tests/test_battle.cpp)
target_include_directories(tp_battle_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(tp_battle_tests PRIVATE tp_game)
target_compile_options(tp_battle_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME tp_battle_tests COMMAND tp_battle_tests)

add_executable(tp_battle_layout_tests
  tests/test_battle_layout.cpp
  src/render/Renderer.cpp
  src/terminal/Terminal.cpp
  src/terminal/Keys.cpp
  src/input/Input.cpp
  src/util/Xdg.cpp
  src/util/Settings.cpp
  src/util/HighScores.cpp
  src/app/SettingsMenu.cpp)
target_include_directories(tp_battle_layout_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(tp_battle_layout_tests PRIVATE tp_game tp_battle)
target_compile_definitions(tp_battle_layout_tests PRIVATE "TP_VERSION=\"${PROJECT_VERSION}\"")
target_compile_options(tp_battle_layout_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME tp_battle_layout_tests COMMAND tp_battle_layout_tests)

add_executable(tp_battle_session_tests tests/test_battle_session.cpp)
target_include_directories(tp_battle_session_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(tp_battle_session_tests PRIVATE tp_game tp_battle)
target_compile_options(tp_battle_session_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME tp_battle_session_tests COMMAND tp_battle_session_tests)

add_executable(tp_battle_codec_tests tests/test_battle_codec.cpp)
target_include_directories(tp_battle_codec_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(tp_battle_codec_tests PRIVATE tp_battle)
target_compile_options(tp_battle_codec_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME tp_battle_codec_tests COMMAND tp_battle_codec_tests
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

if(TP_BATTLE_INSECURE)
  add_executable(tp_battle_insecure_smoke tests/test_battle_insecure_smoke.cpp)
  target_include_directories(tp_battle_insecure_smoke PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_link_libraries(tp_battle_insecure_smoke PRIVATE tp_battle)
  target_compile_options(tp_battle_insecure_smoke PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME tp_battle_insecure_smoke COMMAND tp_battle_insecure_smoke)
endif()

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
