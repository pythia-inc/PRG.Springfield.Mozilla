!macro customInstall
  WriteRegStr SHCTX "SOFTWARE\RegisteredApplications" "Springfield" "Software\Clients\StartMenuInternet\Springfield\Capabilities"

  WriteRegStr SHCTX "SOFTWARE\Classes\Springfield" "" "Springfield HTML Document"
  WriteRegStr SHCTX "SOFTWARE\Classes\Springfield\Application" "AppUserModelId" "Springfield"
  WriteRegStr SHCTX "SOFTWARE\Classes\Springfield\Application" "ApplicationIcon" "$INSTDIR\Springfield.exe,0"
  WriteRegStr SHCTX "SOFTWARE\Classes\Springfield\Application" "ApplicationName" "Springfield"
  WriteRegStr SHCTX "SOFTWARE\Classes\Springfield\Application" "ApplicationCompany" "Pythia"
  WriteRegStr SHCTX "SOFTWARE\Classes\Springfield\Application" "ApplicationDescription" "A privacy-focused, extensible and beautiful web browser"
  WriteRegStr SHCTX "SOFTWARE\Classes\Springfield\DefaultIcon" "DefaultIcon" "$INSTDIR\Springfield.exe,0"
  WriteRegStr SHCTX "SOFTWARE\Classes\Springfield\shell\open\command" "" '"$INSTDIR\Springfield.exe" "%1"'

  WriteRegStr SHCTX "SOFTWARE\Classes\.htm\OpenWithProgIds" "Springfield" ""
  WriteRegStr SHCTX "SOFTWARE\Classes\.html\OpenWithProgIds" "Springfield" ""

  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield" "" "Springfield"
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\DefaultIcon" "" "$INSTDIR\Springfield.exe,0"
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\Capabilities" "ApplicationDescription" "A privacy-focused, extensible and beautiful web browser"
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\Capabilities" "ApplicationName" "Springfield"
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\Capabilities" "ApplicationIcon" "$INSTDIR\Springfield.exe,0"
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\Capabilities\FileAssociations" ".htm" "Springfield"
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\Capabilities\FileAssociations" ".html" "Springfield"
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\Capabilities\URLAssociations" "http" "Springfield"
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\Capabilities\URLAssociations" "https" "Springfield"
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\Capabilities\StartMenu" "StartMenuInternet" "Springfield"
  
  WriteRegDWORD SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\InstallInfo" "IconsVisible" 1
  
  WriteRegStr SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield\shell\open\command" "" "$INSTDIR\Springfield.exe"
!macroend
!macro customUnInstall
  DeleteRegKey SHCTX "SOFTWARE\Classes\Springfield"
  DeleteRegKey SHCTX "SOFTWARE\Clients\StartMenuInternet\Springfield"
  DeleteRegValue SHCTX "SOFTWARE\RegisteredApplications" "Springfield"
!macroend