idf_component_register(SRC_DIRS src
                                src/comdrivers
                                src/devdrivers
                                src/dispdrivers
                                src/emudevs
                                src/network
                       INCLUDE_DIRS src
                                src/fonts
                                src/images
                                src/comdrivers
                                src/devdrivers
                                src/dispdrivers
                                src/emudevs
                                src/network
                       REQUIRES fatfs
                                spiffs
                                ulp
                                esp_timer
                                spi_flash
                                esp_wifi
                                nvs_flash
                                soc)