MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

SUBDIRLIST(plugin_types ${plugins_directory})

FOREACH(plugin_type ${plugin_types})
    SUBDIRLIST(plugins ${plugins_directory}/${plugin_type})
    FOREACH(plugin ${plugins})
        file(GLOB_RECURSE plugin_src_files ${plugins_directory}/${plugin_type}/${plugin}/*.cpp)
        add_library(${plugin} SHARED ${plugin_src_files})
        set_target_properties(${plugin} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/plugins/${plugin_type})
        target_include_directories(${plugin} SYSTEM PRIVATE
            ${Qt6Xml_INCLUDE_DIRS}
            ${Qt6WebSockets_INCLUDE_DIRS}
            ${Qt6SerialBus_INCLUDE_DIRS}
            ${Qt6Multimedia_INCLUDE_DIRS}
            ${Qt6MultimediaWidgets_INCLUDE_DIRS}
            ${Qt6Widgets_INCLUDE_DIRS}
            ${Qt6Bluetooth_INCLUDE_DIRS}
            ${Qt6DBus_INCLUDE_DIRS}
#	    ${Qt5Concurrent_INCLUDE_DIRS}
            ${Boost_INCLUDE_DIRS}
            ${LIBUSB_1_INCLUDE_DIRS}
            ${PROTOBUF_INCLUDE_DIR}
            ${OPENSSL_INCLUDE_DIR}
            ${RTAUDIO_INCLUDE_DIRS}
            ${AASDK_PROTO_INCLUDE_DIRS}
            ${AASDK_INCLUDE_DIRS}
            ${OPENAUTO_INCLUDE_DIRS}
            ${BCM_HOST_INCLUDE_DIRS}
            ${ILCLIENT_INCLUDE_DIRS}
            ${GST_INCLUDE_DIRS}
            ${QTGSTREAMER_INCLUDE_DIR}
            ${TAGLIB_INCLUDE_DIRS}
            ${X11_INCLUDE_DIR}
            ${GPIOD_INCLUDE_DIRS}
            ${include_directory}
        )
        target_link_libraries(${plugin}
            aasdk_proto
            aasdk
            openauto
            ${Qt6DBus_LIBRARIES}
            ${Boost_LIBRARIES}
            ${Qt6Xml_LIBRARIES}
            ${Qt6WebSockets_LIBRARIES}
            ${Qt6SerialBus_LIBRARIES}
            ${Qt6Multimedia_LIBRARIES}
            ${Qt6MultimediaWidgets_LIBRARIES}
            ${Qt6Bluetooth_LIBRARIES}
#            ${Qt5Concurrent_LIBRARIES}
            KF6::BluezQt
            ${LIBUSB_1_LIBRARIES}
            ${PROTOBUF_LIBRARIES}
            ${BCM_HOST_LIBRARIES}
            ${ILCLIENT_LIBRARIES}
            ${WINSOCK2_LIBRARIES}
            ${RTAUDIO_LIBRARIES}
            ${GST_LIBRARIES}
            ${QTGLIB_LIBRARY}
            ${QTGLIB_LIBRARIES}
            ${QTGSTREAMER_LIBRARY}
            ${QTGSTREAMER_LIBRARIES}
            ${QTGSTREAMER_UI_LIBRARY}
            ${QTGSTREAMER_UI_LIBRARIES}
            ${Qt6QuickWidgets_LIBRARIES}
            ${QTGSTREAMER_QUICK_LIBRARIES}
            ${QTGSTREAMER_QUICK_LIBRARY}
            ${TAGLIB_LIBRARIES}
            ${X11_LIBRARIES}
            ${GPIOD_LIBRARIES}
        )
    ENDFOREACH()
ENDFOREACH()
