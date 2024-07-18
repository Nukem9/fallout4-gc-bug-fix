vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO alandtse/CommonLibF4
        REF a22ea1bae7f31e8d8467713d9a57852518bb8a5c
        SHA512 5f462c88c5b2dd7a9bc6adc0e1aae9f95547fcf8c632319365f1d5423ecf38b244d9009149a89ccc9f9bdb7bdc25cae61e612cc0b744da3d246ce32beb9d94c5
        HEAD_REF master
        PATCHES
                "f4se_include_fix.patch"
)

vcpkg_configure_cmake(
        SOURCE_PATH "${SOURCE_PATH}/CommonLibF4"
        PREFER_NINJA
)

vcpkg_install_cmake()
vcpkg_cmake_config_fixup(PACKAGE_NAME "CommonLibF4-ng" CONFIG_PATH "lib/cmake/CommonLibF4")
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
