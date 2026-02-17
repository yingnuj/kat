add_executable(spi_dma spi_dma.c)

target_link_libraries(spi_dma pico_stdlib hardware_spi hardware_dma
                      hardware_irq)

# enable usb output, disable uart output
pico_enable_stdio_usb(spi_dma 1)
pico_enable_stdio_uart(spi_dma 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(spi_dma)