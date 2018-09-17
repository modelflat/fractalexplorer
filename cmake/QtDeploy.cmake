# qt deploy & create installer simplified functionality

get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)

execute_process(
    COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
    RESULT_VARIABLE return_code
    OUTPUT_VARIABLE qt5_install_prefix
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Qt install prefix: ${qt5_install_prefix}")

set(windeployqt_imported_location "${qt5_install_prefix}/bin/windeployqt.exe")
if (EXISTS ${windeployqt_imported_location})
    add_executable(Qt5::windeployqt IMPORTED)
    set_target_properties(Qt5::windeployqt PROPERTIES IMPORTED_LOCATION ${windeployqt_imported_location})
    message(STATUS "WinDeployQt location: ${windeployqt_imported_location}")
endif ()


function(deploy_target
         _target_name
         _deploy_dir
         # _deploy_args
         )
    separate_arguments(_deploy_args NATIVE_COMMAND "${ARGV2}")

    # More info on windeployqt http://doc.qt.io/qt-5/windows-deployment.html

    if (TARGET Qt5::windeployqt)
        if (${_deploy_dir})
            add_custom_command(TARGET ${_target_name}
                               POST_BUILD
                               COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/${_deploy_dir}"
                               COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
                               COMMAND Qt5::windeployqt --dir "${CMAKE_CURRENT_BINARY_DIR}/${_deploy_dir}" ${_deploy_args} "$<TARGET_FILE_DIR:${_target_name}>/$<TARGET_FILE_NAME:${_target_name}>"
                               COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:${_target_name}>" "${CMAKE_CURRENT_BINARY_DIR}/${_deploy_dir}"
                               )
        else ()
            # Deploy directory points to ${CMAKE_CURRENT_BINARY_DIR}, do not remove it or copy executable into it
            add_custom_command(TARGET ${_target_name}
                               POST_BUILD
                               COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
                               COMMAND Qt5::windeployqt --dir "${CMAKE_CURRENT_BINARY_DIR}/${_deploy_dir}" ${_deploy_args} "$<TARGET_FILE_DIR:${_target_name}>/$<TARGET_FILE_NAME:${_target_name}>"
                               )
        endif ()
    else ()
        message(ERROR "Cannot deploy application: windeployqt.exe not found")
    endif ()
endfunction(deploy_target)

function(create_target_installer
         _target_name
         _installer_config_xml_path
         _package_xml_path
         _package_full_name
         # _deploy_args
         )

    set(_deploy_args ${ARGV4})

    set(_qtifw_home "$ENV{QT_HOME}/Tools/QtInstallerFramework/3.0")# TODO implement more clever lookup
    set(binarycreator_imported_location "${_qtifw_home}/bin/binarycreator.exe")
    message(STATUS "QtInstallerFramework's binarycreator: ${binarycreator_imported_location}")
    if (EXISTS ${binarycreator_imported_location})
        add_executable(QtIFW::binarycreator IMPORTED)
        set_target_properties(QtIFW::binarycreator PROPERTIES IMPORTED_LOCATION ${binarycreator_imported_location})
        message(STATUS "BinaryCreator location: ${binarycreator_imported_location}")
    endif ()

    # First, deploy into ${CMAKE_CURRENT_BINARY_DIR}/deploy
    deploy_target(${_target_name} deploy ${_deploy_args})

    set(_temp "${CMAKE_CURRENT_BINARY_DIR}/packages/${_package_full_name}")

    # Then package using binarycreator ( http://doc.qt.io/qtinstallerframework/ifw-tools.html#using-binarycreator )
    if (TARGET QtIFW::binarycreator)
        add_custom_command(TARGET ${_target_name}
                           POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E make_directory "${_temp}/meta"
                           COMMAND ${CMAKE_COMMAND} -E copy "${_package_xml_path}" "${_temp}/meta"
                           COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/deploy" "${_temp}/data"
                           COMMAND QtIFW::binarycreator -f -p "${CMAKE_CURRENT_BINARY_DIR}/packages" -c "${_installer_config_xml_path}" "${_target_name}-Installer.exe"
                           COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/packages"
                           )
    else ()
        message(FATAL_ERROR "Cannot create installer: binarycreator.exe not found")
    endif ()

endfunction(create_target_installer)