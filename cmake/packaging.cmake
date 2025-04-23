# set some project metadata for the installer
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "Coati Software")
set(CPACK_PACKAGE_CONTACT "eng.ahmedhussein89@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Interactive Source Explorer")
set(CPACK_PACKAGE_DESCRIPTION
    "Sourcetrail is an interactive source code explorer that helps you understand, refactor, and share C, C++, and Java code."
)
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})
set(CPACK_VERBATIM_VARIABLES YES)

# if we not call the line below, groups won't show properlly in the GUI installer
set(CPACK_COMPONENTS_GROUPING IGNORE)

# we are interested in cross-platform support. IFW is the only generator that allows for it, so set it explicitly
set(CPACK_GENERATOR "IFW")

# set some IFW specific variables, which can be derived from the more generic variables given above
set(CPACK_IFW_VERBOSE ON)
set(CPACK_IFW_PACKAGE_TITLE ${CPACK_PACKAGE_NAME})
set(CPACK_IFW_PACKAGE_PUBLISHER ${CPACK_PACKAGE_VENDOR})
set(CPACK_IFW_PRODUCT_URL "https://opensourcesourcetrail.github.io/")

# create a more memorable name for the maintenance tool (used for uninstalling the package)
set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME ${PROJECT_NAME}_MaintenanceTool)
set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_INI_FILE ${CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME}.ini)

# customise the theme if required
set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Modern")

# adjust the default size
set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT 400)

# set the installer logo
set(CPACK_IFW_PACKAGE_LOGO ${CMAKE_SOURCE_DIR}/images/logo_128_128.png)

# now include the relevant (cross-platform) packaging script
include(CPack)
include(CPackIFW)

# add the binary as a component to the installer
cpack_add_component(
  Sourcetrail REQUIRED
  DISPLAY_NAME "Sourcetrail Application"
  DESCRIPTION "Install Sourcetrail Application")

cpack_ifw_configure_component(Sourcetrail REQUIRED LICENSES "GNU License" ${CMAKE_SOURCE_DIR}/LICENSE.txt)
