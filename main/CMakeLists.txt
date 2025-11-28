idf_component_register(SRCS "hello_world_main.cpp"
                        "bsp/audio.c"
                        "bsp/camera.c"
                        "bsp/lcd.c"
                        "face/who_ai_utils.cpp"
                        "face/who_human_face_detection.cpp"
                        "sd_card/sd_card.c"
                        "bsp/adc_button.c"
                    INCLUDE_DIRS "."
                    "bsp"
                    "face"
                    "sd_card"
                    REQUIRES 
                    "esp_timer"
                    "fatfs"
                    )
set(EXTRA_COMPONENT_DIRS 
    $ENV{IDF_PATH}/components/fatfs)