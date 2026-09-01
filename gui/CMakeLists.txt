enable_language(C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(VTE REQUIRED IMPORTED_TARGET vte-2.91-gtk4)

add_executable(terminalpolyominos-gui
  main.c
  launcher.c
  paths.c)

target_compile_options(terminalpolyominos-gui PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_compile_definitions(terminalpolyominos-gui PRIVATE "TP_VERSION=\"${PROJECT_VERSION}\"")

target_link_libraries(terminalpolyominos-gui PRIVATE
  PkgConfig::GTK4
  PkgConfig::VTE)

install(TARGETS terminalpolyominos-gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
