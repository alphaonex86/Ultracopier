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
    HelpDialog.ui
    PluginInformation.ui
    OptionDialog.ui
    OSSpecific.ui
    ProductKey.ui
    plugins/CopyEngine/Ultracopier-Spec/copyEngineOptions.ui
    plugins/CopyEngine/Ultracopier-Spec/debugDialog.ui
    plugins/CopyEngine/Ultracopier-Spec/DiskSpace.ui
    plugins/CopyEngine/Ultracopier-Spec/fileErrorDialog.ui
    plugins/CopyEngine/Ultracopier-Spec/fileExistsDialog.ui
    plugins/CopyEngine/Ultracopier-Spec/fileIsSameDialog.ui
    plugins/CopyEngine/Ultracopier-Spec/FilterRules.ui
    plugins/CopyEngine/Ultracopier-Spec/Filters.ui
    plugins/CopyEngine/Ultracopier-Spec/folderExistsDialog.ui
    plugins/CopyEngine/Ultracopier-Spec/RenamingRules.ui
    plugins/Themes/Oxygen/interface.ui
    plugins/Themes/Oxygen/options.ui
    plugins/Themes/Oxygen/themesOptions.ui
    ResourcesManager.cpp
    DebugEngine.cpp
    OptionEngine.cpp
    PluginsManager.cpp
    LanguagesManager.cpp
    PluginInformation.cpp
    lib/qt-tar-xz/QXzDecodeThread.cpp
    lib/qt-tar-xz/QXzDecode.cpp
    lib/qt-tar-xz/QTarDecode.cpp
    lib/qt-tar-xz/xz_crc32.c
    lib/qt-tar-xz/xz_dec_stream.c
    lib/qt-tar-xz/xz_dec_lzma2.c
    lib/qt-tar-xz/xz_dec_bcj.c
    SessionLoader.cpp
    ExtraSocket.cpp
    CopyListener.cpp
    CopyEngineManager.cpp
    Core.cpp
    PluginLoaderCore.cpp
    OptionDialog.cpp
    LocalPluginOptions.cpp
    LocalListener.cpp
    CliParser.cpp
    FacilityEngine.cpp
    FacilityEngineVersion.cpp
    LogThread.cpp
    OSSpecific.cpp
    cpp11addition.cpp
    DebugModel.cpp
    InternetUpdater.cpp
    cpp11additionstringtointcpp.cpp
    ProductKey.cpp
    plugins/CopyEngine/Ultracopier-Spec/CallBackEventLoop.h
    plugins/CopyEngine/Ultracopier-Spec/CompilerInfo.h
    plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.h
    plugins/CopyEngine/Ultracopier-Spec/CopyEngine.h
    plugins/CopyEngine/Ultracopier-Spec/CopyEngineUltracopier-SpecVariable.h
    plugins/CopyEngine/Ultracopier-Spec/DebugDialog.h
    plugins/CopyEngine/Ultracopier-Spec/DebugEngineMacro.h
    plugins/CopyEngine/Ultracopier-Spec/DiskSpace.h
    plugins/CopyEngine/Ultracopier-Spec/DriveManagement.h
    plugins/CopyEngine/Ultracopier-Spec/Environment.h
    plugins/CopyEngine/Ultracopier-Spec/EventLoop.h
    plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.h
    plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.h
    plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.h
    plugins/CopyEngine/Ultracopier-Spec/FilterRules.h
    plugins/CopyEngine/Ultracopier-Spec/Filters.h
    plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.h
    plugins/CopyEngine/Ultracopier-Spec/ListThread.h
    plugins/CopyEngine/Ultracopier-Spec/MkPath.h
    plugins/CopyEngine/Ultracopier-Spec/ports.h
    plugins/CopyEngine/Ultracopier-Spec/RenamingRules.h
    plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.h
    plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition_CopyEngine.h
    plugins/CopyEngine/Ultracopier-Spec/StructEnumDefinition.h
    plugins/CopyEngine/Ultracopier-Spec/TransferThread.h
    plugins/CopyEngine/Ultracopier-Spec/CopyEngine-collision-and-error.cpp
    plugins/CopyEngine/Ultracopier-Spec/CopyEngine.cpp
    plugins/CopyEngine/Ultracopier-Spec/DebugDialog.cpp
    plugins/CopyEngine/Ultracopier-Spec/DiskSpace.cpp
    plugins/CopyEngine/Ultracopier-Spec/DriveManagement.cpp
    plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.cpp
    plugins/CopyEngine/Ultracopier-Spec/FileErrorDialog.cpp
    plugins/CopyEngine/Ultracopier-Spec/FileExistsDialog.cpp
    plugins/CopyEngine/Ultracopier-Spec/FileIsSameDialog.cpp
    plugins/CopyEngine/Ultracopier-Spec/FilterRules.cpp
    plugins/CopyEngine/Ultracopier-Spec/Filters.cpp
    plugins/CopyEngine/Ultracopier-Spec/FolderExistsDialog.cpp
    plugins/CopyEngine/Ultracopier-Spec/MkPath.cpp
    plugins/CopyEngine/Ultracopier-Spec/RenamingRules.cpp
    plugins/CopyEngine/Ultracopier-Spec/ScanFileOrFolder.cpp
    plugins/CopyEngine/Ultracopier-Spec/TransferThread.cpp
    plugins/CopyEngine/Ultracopier-Spec/ListThread.cpp
    plugins/CopyEngine/Ultracopier-Spec/ListThreadActions.cpp
    plugins/CopyEngine/Ultracopier-Spec/ListThreadMedia.cpp
    plugins/CopyEngine/Ultracopier-Spec/ListThreadNew.cpp
    plugins/CopyEngine/Ultracopier-Spec/ListThreadOptions.cpp
    plugins/CopyEngine/Ultracopier-Spec/ListThreadScan.cpp
    plugins/CopyEngine/Ultracopier-Spec/ListThreadStat.cpp
    plugins/CopyEngine/Ultracopier-Spec/async/ReadThread.cpp
    plugins/CopyEngine/Ultracopier-Spec/async/WriteThread.cpp
    plugins/Listener/catchcopy-v0002/listener.cpp
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ClientCatchcopy.cpp
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ExtraSocketCatchcopy.cpp
    plugins/Listener/catchcopy-v0002/catchcopy-api-0002/ServerCatchcopy.cpp
    plugins/Themes/Oxygen/interface.cpp
    plugins/Themes/Oxygen/ThemesFactory.cpp
    plugins/Themes/Oxygen/TransferModel.cpp
    plugins/CopyEngine/Ultracopier-Spec/async/TransferThreadAsync.cpp
    interface/FacilityInterface.h
    interface/OptionInterface.h
    interface/PluginInterface_CopyEngine.h
    interface/PluginInterface_Listener.h
    interface/PluginInterface_PluginLoader.h
    interface/PluginInterface_SessionLoader.h
    interface/PluginInterface_Themes.h
    ThemesManager.cpp
    main.cpp
    EventDispatcher.cpp
    SystrayIcon.cpp
    HelpDialog.cpp
    resources/ultracopier-resources.qrc
    resources/ultracopier-resources_unix.qrc
    plugins/static-plugins.qrc
    plugins/static-plugins-oxygen.qrc
    plugins/CopyEngine/Ultracopier-Spec/copyEngineResources.qrc
    plugins/Themes/Oxygen/interfaceResources.qrc
)
add_definitions(-DNOAUDIO -DULTRACOPIER_PLUGIN_ALL_IN_ONE -DULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT -DULTRACOPIER_NODEBUG)

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
