# translate_add_sources(
#   QtCurve.css.in
#   QtCurve-KDEButtonOrder.css.in
#   preferences-rev.xml)

if(NOT ENABLE_GTK2)
  return()
endif()

configure_file(QtCurve.css.in QtCurve.css)
configure_file(QtCurve-KDEButtonOrder.css.in QtCurve-KDEButtonOrder.css)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/QtCurve.css" preferences-rev.xml
  "${CMAKE_CURRENT_BINARY_DIR}/QtCurve-KDEButtonOrder.css"
  DESTINATION "${GTK2_THEME_DIR}/mozilla")
