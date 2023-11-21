#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/usb_serial_jtag_ll.h"


#define BUFFER_LEN                  (1024)
#define MIN_STRING_LENGTH           (60)
#define MAX_STRING_LENGTH           (70)


static uint8_t forward_buffer[BUFFER_LEN] = { 0 };


extern "C" void app_main(void)
{
    // Configure USB-CDC
    usb_serial_jtag_driver_config_t config =
    {
        .tx_buffer_size = 1024,
        .rx_buffer_size = 1024,
    };

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));

    uint16_t string_length = MIN_STRING_LENGTH;
    while (true)
    {
        // Create a string to send via USB. The string has a header (detailing how long the string is in bytes) and
        // is padded with a "._" pattern. will be in the format:
        // `20 bytes:_._._._._.\n`
        // Note: the string is not null terminated.
        size_t header_length = snprintf((char *)forward_buffer, string_length, "%d bytes:", string_length);
        for (size_t idx = header_length; idx < string_length - 1; idx++)
        {
            forward_buffer[idx] = (idx % 2 == 0) ? '.' : '_';
        }
        forward_buffer[string_length - 1] = '\n';

        // Send all of the bytes by USB.
        size_t bytes_written = 0;
        while (bytes_written < string_length)
        {
            size_t bytes_left = string_length - bytes_written;
            bytes_written += usb_serial_jtag_write_bytes(
                forward_buffer + bytes_written, bytes_left, 500 / portTICK_PERIOD_MS);
            usb_serial_jtag_ll_txfifo_flush();
        }

        if (++string_length > MAX_STRING_LENGTH)
        {
            string_length = MIN_STRING_LENGTH;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
