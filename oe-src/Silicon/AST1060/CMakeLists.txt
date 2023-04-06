set(AMI_PORT_ROOT ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "AMI_PORT_ROOT")

zephyr_interface_library_named(ami_middleware)
zephyr_library()

set(AMI_MIDDLEWARE_ROOT ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "AMI_MIDDLEWARE_ROOT")
zephyr_library_sources(
    ${AMI_MIDDLEWARE_ROOT}/flash/flash_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/gpio/gpio_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/crypto/hash_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/crypto/rsa_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/crypto/ecdsa_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/i2c/i2c_filter_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/i2c/I2C_Slave_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/watchdog/watchdog_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/otp/otp_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/abr/abr_aspeed.c
    ${AMI_MIDDLEWARE_ROOT}/spi_filter/spi_filter_aspeed.c
)
zephyr_library_include_directories(
    .
    $ENV{ZEPHYR_BASE}/build/zephyr/include/generated/
    $ENV{ZEPHYR_BASE}/include/storage/
)

zephyr_library_link_libraries(ami_middleware)
