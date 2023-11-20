#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/usb_serial_jtag_ll.h"

#include <stdlib.h>


#define DELAY_SHORT                 (20 / portTICK_PERIOD_MS)
#define DELAY_LONG                  (1000 / portTICK_PERIOD_MS)
#define BUFFER_LEN                  (1024)

#define UART_BUF_SIZE               (1024)
#define UART_TIMEOUT                (300 / portTICK_PERIOD_MS)

#define MIN_STRING_LENGTH           (60)
#define MAX_STRING_LENGTH           (70)


static uint8_t forward_buffer[BUFFER_LEN] = { 0 };


static size_t usb_write(const uint8_t *buffer, size_t length);
static void usb_send(void);


extern "C" void app_main(void)
{
    // Short delay for system to settle
    vTaskDelay(DELAY_SHORT);

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
            bytes_written += usb_write(forward_buffer + bytes_written, bytes_left);
            usb_send();
        }

        if (++string_length > MAX_STRING_LENGTH)
        {
            string_length = MIN_STRING_LENGTH;
        }

        vTaskDelay(DELAY_LONG);
    }
}


static size_t usb_write(const uint8_t *buffer, size_t length)
{
    int can_write = usb_serial_jtag_ll_txfifo_writable();
    while (!can_write)
    {
        vTaskDelay(1);
        can_write = usb_serial_jtag_ll_txfifo_writable();
    }
    return (size_t)usb_serial_jtag_ll_write_txfifo(buffer, length);
}


static void usb_send(void)
{
    usb_serial_jtag_ll_txfifo_flush();
}
