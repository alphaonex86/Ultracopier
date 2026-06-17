!include Library.nsh
!define LIBRARY_X64
!define LIBRARY_SHELL_EXTENSION
!define LIBRARY_COM

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Ultracopier"
!define PRODUCT_VERSION "X.X.X.X"
!define PRODUCT_PUBLISHER "Ultracopier"
!define PRODUCT_WEB_SITE "http://ultracopier.first-world.info/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\ultracopier.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

RequestExecutionLevel admin

SetCompressor /FINAL /SOLID lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
; !insertmacro MUI_PAGE_LICENSE "COPYING.txt"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\ultracopier.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "setup.exe"
InstallDir "$PROGRAMFILES\Ultracopier"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Section "SectionPrincipale" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite on
  File "ultracopier.exe"
  CreateDirectory "$SMPROGRAMS\Ultracopier"
  CreateShortCut "$SMPROGRAMS\Ultracopier\Ultracopier.lnk" "$INSTDIR\ultracopier.exe"
  File /r /x *.nsi /x setup.exe *
  ; Remove the stale pre-rename plugin dirs left by older installs (renamed catchcopy-v0002
  ; -> catchcopy). The old uninstaller's "RMDir /r $INSTDIR" cannot delete the catchcopy shell
  ; DLL while explorer.exe has it loaded, so the old dir survives and Ultracopier loads BOTH
  ; as duplicate Listener/PluginLoader plugins -> wrong "semi-uncaught" tray icon. Unregister
  ; + remove the old dirs here, BEFORE registering the new DLLs. /REBOOTOK handles locked DLLs.
  ExecWait 'regsvr32 /s /u "$INSTDIR\PluginLoader\catchcopy-v0002\catchcopy32.dll"'
  ExecWait 'regsvr32 /s /u "$INSTDIR\PluginLoader\catchcopy-v0002\catchcopy64.dll"'
  ExecWait 'regsvr32 /s /u "$INSTDIR\PluginLoader\catchcopy-v0002\catchcopy32all.dll"'
  ExecWait 'regsvr32 /s /u "$INSTDIR\PluginLoader\catchcopy-v0002\catchcopy64all.dll"'
  RMDir /r /REBOOTOK "$INSTDIR\PluginLoader\catchcopy-v0002"
  RMDir /r /REBOOTOK "$INSTDIR\Listener\catchcopy-v0002"
  !insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_PROTECTED PluginLoader\catchcopy\catchcopy32.dll $INSTDIR\PluginLoader\catchcopy\catchcopy32.dll $INSTDIR
  !insertmacro InstallLib REGDLL NOTSHARED NOREBOOT_PROTECTED PluginLoader\catchcopy\catchcopy64.dll $INSTDIR\PluginLoader\catchcopy\catchcopy64.dll $INSTDIR
SectionEnd

Section -AdditionalIcons
  CreateShortCut "$SMPROGRAMS\Ultracopier\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\ultracopier.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\ultracopier.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd


Function un.onUninstFailed
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "To remove $(^Name) from the computer, close the application and remove manualy the folder"
FunctionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) have been uninstall from the computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely uninstall $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Function .onInit
 
  ReadRegStr $R0 HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
  "UninstallString"
  StrCmp $R0 "" done
 
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  Abort
 
;Run the uninstaller
uninst:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR' ;Do not copy the uninstaller to a temp file
 
  IfErrors no_remove_uninstaller done
    ;You can either use Delete /REBOOTOK in the uninstaller or add some code
    ;here to remove the uninstaller. Use a registry key to check
    ;whether the user has chosen to uninstall. If you are using an uninstaller
    ;components page, make sure all sections are uninstalled.
  no_remove_uninstaller:
 
done:
 
FunctionEnd

Section Uninstall
  IfFileExists "$INSTDIR\ultracopier.exe" CloseProgram
  Abort "The original application $INSTDIR\ultracopier.exe is not found"
  Goto NotLaunched
  CloseProgram:
  ExecWait '"$INSTDIR\ultracopier.exe" quit' $0
  IntCmp $0 0 NotLaunched
  DetailPrint "Waiting Close..."
  CloseLoop:
    Sleep 200
    ExecWait '"$INSTDIR\ultracopier.exe" quit' $0
    IntCmp $0 0 NotLaunched
    Goto CloseLoop

  NotLaunched:

  ExecWait 'regsvr32 /s /u "$INSTDIR\PluginLoader\catchcopy\catchcopy32.dll"'
  ExecWait 'regsvr32 /s /u "$INSTDIR\PluginLoader\catchcopy\catchcopy64.dll"'

; DeleteRegKey HKCU "Software\Ultracopier"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "ultracopier"
  Delete "$SMPROGRAMS\Ultracopier\Uninstall.lnk"
  Delete "$SMPROGRAMS\Ultracopier\Ultracopier.lnk"

  Delete /REBOOTOK $SMPROGRAMS\PluginLoader\catchcopy\catchcopy32.dll
  Delete /REBOOTOK $SMPROGRAMS\PluginLoader\catchcopy\catchcopy64.dll
  Delete /REBOOTOK $INSTDIR\PluginLoader\catchcopy\catchcopy32.dll
  Delete /REBOOTOK $INSTDIR\PluginLoader\catchcopy\catchcopy64.dll
  RMDir /REBOOTOK /r "$SMPROGRAMS\Ultracopier"
  RMDir /REBOOTOK /r "$INSTDIR"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
