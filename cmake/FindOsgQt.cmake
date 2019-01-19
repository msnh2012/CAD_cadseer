FIND_PATH(OSGQT_INCLUDE_DIR GraphicsWindowQt
  /usr/include/osgQt
  /usr/local/include/osgQt
  "C:\\Users\\holden\\development\\vcpkg\\installed\\x64-windows\\include\\osgQt"
)
FIND_LIBRARY(OSGQT_LIBRARY osgQt5
  /usr/lib
  /usr/local/lib
  "C:\\Users\\holden\\development\\vcpkg\\installed\\x64-windows\\lib"
)


include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSGQT REQUIRED_VARS OSGQT_INCLUDE_DIR VERSION_VAR)
