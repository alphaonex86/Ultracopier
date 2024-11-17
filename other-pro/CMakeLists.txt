cmake_minimum_required(VERSION 3.5)

project(Ultracopier VERSION 3.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Xml)

set(PROJECT_SOURCES
    ../little/main-little.cpp
    ../plugins/Themes/Oxygen/interface.cpp
    ../plugins/Themes/Oxygen/ThemesFactory.cpp
    ../plugins/Themes/Oxygen/TransferModel.cpp
    ../little/OptionsEngineLittle.cpp
    ../FacilityEngine.cpp
    ../cpp11addition.cpp
    ../cpp11additionstringtointcpp.cpp
    ../plugins/Themes/Oxygen/interfaceResources_unix.qrc
    ../plugins/Themes/Oxygen/interfaceResources_windows.qrc
    ../plugins/Themes/Oxygen/interfaceResources.qrc
    ../plugins/Themes/Oxygen/themesOptions.ui
    ../plugins/Themes/Oxygen/options.ui
    ../plugins/Themes/Oxygen/interface.ui
    ../plugins/Themes/Oxygen/DebugEngineMacro.h
    ../plugins/Themes/Oxygen/Environment.h
    ../plugins/Themes/Oxygen/interface.h
    ../plugins/Themes/Oxygen/OxygenVariable.h
    ../plugins/Themes/Oxygen/StructEnumDefinition.h
    ../plugins/Themes/Oxygen/ThemesFactory.h
    ../plugins/Themes/Oxygen/TransferModel.h
    ../little/OptionsEngineLittle.h
    ../FacilityEngine.h
    ../interface/FacilityInterface.h
    ../interface/OptionInterface.h
    ../interface/PluginInterface_CopyEngine.h
    ../interface/PluginInterface_SessionLoader.h
    ../interface/PluginInterface_Themes.h
    ../interface/PluginInterface_Listener.h
    ../interface/PluginInterface_PluginLoader.h
    ../cpp11addition.h
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngine-collision-and-error.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngine.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/DebugDialog.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/DiskSpace.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/DriveManagement.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/FilterRules.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/Filters.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ListThread_InodeAction.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/MkPath.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/TransferThread.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ListThread.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadActions.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadStat.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadScan.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadOptions.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadNew.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadMedia.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/ListThreadListChange.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/async/ReadThread.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/async/WriteThread.cpp
    ../plugins/CopyEngine/Ultracopier-Spec/copyEngineResources.qrc
    ../plugins/CopyEngine/Ultracopier-Spec/copyEngineOptions.ui
    ../plugins/CopyEngine/Ultracopier-Spec/debugDialog.ui
    ../plugins/CopyEngine/Ultracopier-Spec/DiskSpace.ui
    ../plugins/CopyEngine/Ultracopier-Spec/fileErrorDialog.ui
    ../plugins/CopyEngine/Ultracopier-Spec/fileExistsDialog.ui
    ../plugins/CopyEngine/Ultracopier-Spec/fileIsSameDialog.ui
    ../plugins/CopyEngine/Ultracopier-Spec/FilterRules.ui
    ../plugins/CopyEngine/Ultracopier-Spec/Filters.ui
    ../plugins/CopyEngine/Ultracopier-Spec/folderExistsDialog.ui
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.ui
    ../plugins/CopyEngine/Ultracopier-Spec/CompilerInfo.h
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngine.h
    ../plugins/CopyEngine/Ultracopier-Spec/DebugDialog.h
    ../plugins/CopyEngine/Ultracopier-Spec/DebugEngineMacro.h
    ../plugins/CopyEngine/Ultracopier-Spec/DiskSpace.h
    ../plugins/CopyEngine/Ultracopier-Spec/DriveManagement.h
    ../plugins/CopyEngine/Ultracopier-Spec/Environment.h
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.h
    ../plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.h
    ../plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.h
    ../plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.h
    ../plugins/CopyEngine/Ultracopier-Spec/FilterRules.h
    ../plugins/CopyEngine/Ultracopier-Spec/Filters.h
    ../plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.h
    ../plugins/CopyEngine/Ultracopier-Spec/MkPath.h
    ../plugins/CopyEngine/Ultracopier-Spec/ListThread.h
    ../plugins/CopyEngine/Ultracopier-Spec/RenamingRules.h
    ../plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.h
    ../plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition_CopyEngine.h
    ../plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition.h
    ../plugins/CopyEngine/Ultracopier-Spec/TransferThread.h
    ../plugins/CopyEngine/Ultracopier-Spec/CopyEngineUltracopier-SpecVariable.h
    ../plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.h
    ../plugins/CopyEngine/Ultracopier-Spec/async/ReadThread.h
    ../plugins/CopyEngine/Ultracopier-Spec/async/WriteThread.h
)
add_definitions(-DNOAUDIO -DULTRACOPIER_LITTLE -DULTRACOPIER_PLUGIN_ALL_IN_ONE -DULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT -DULTRACOPIER_NODEBUG)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Ultracopier
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
    set_property(TARGET Ultracopier APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Ultracopier SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Ultracopier
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Ultracopier PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(Ultracopier PRIVATE Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(Ultracopier PRIVATE Qt${QT_VERSION_MAJOR}::Xml)

set_target_properties(Ultracopier PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER ultracopier.herman-brule.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS Ultracopier
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Ultracopier)
endif()
