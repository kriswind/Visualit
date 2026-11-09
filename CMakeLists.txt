cmake_minimum_required(VERSION 3.16)

project(visualit
    VERSION 0.1.0
    DESCRIPTION "Git branch visualization dashboard"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Gui Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(visualit
    main.cpp
)

set_source_files_properties(Theme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(visualit
    URI VisualitUi
    SOURCES
        branchmodel.cpp
        branchmodel.h
        commitgraph.cpp
        commitgraph.h
        commitmodel.cpp
        commitmodel.h
        gitrepository.cpp
        gitrepository.h
    QML_FILES
        AppToolBar.qml
        BranchPanel.qml
        CommitDelegate.qml
        CommitDetails.qml
        CommitList.qml
        CompactCommitDelegate.qml
        Main.qml
        RefChip.qml
        StatCard.qml
        StatsBar.qml
        StatusBar.qml
        Theme.qml
)

qt_add_resources(visualit "visualit_icons"
    PREFIX /
    FILES
        icons/visualit.svg
)

target_link_libraries(visualit
    PRIVATE
        Qt6::Gui
        Qt6::Quick
        Qt6::QuickControls2
)

install(TARGETS visualit
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# Absolute Exec path so launchers work even when the session PATH lacks the install bindir.
configure_file(visualit.desktop.in visualit.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/visualit.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(FILES icons/visualit.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

# Short command alias: `vis` -> visualit
install(CODE "
    set(vis_link \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/vis\")
    file(CREATE_LINK visualit \"\${vis_link}\" SYMBOLIC)
    message(STATUS \"Installing: \${vis_link} -> visualit\")
")
