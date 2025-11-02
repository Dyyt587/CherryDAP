/**
 * @file USB_handle.c
 * @brief Handle all Standard Device Requests on endpoint 0
 * @version 0.1
 * @date 2020-01-23
 *
 * @copyright Copyright (c) 2020
 *
 */
#include <stdint.h>
#include <string.h>

#include "main/usbip_server.h"
// #include "main/include/wifi_configuration.h"

#include "USBIP/usb_handle.h"
#include "USBIP/usb_descriptor.h"
#include "USBIP/MSOS20_descriptor.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "esp_log.h"

static char TAG[] = "USBIP";
const char *strings_list[] = {
        0, // reserved: available languages  -> iInterface
        "MGodmonkey",
        "ESP32 DAPLink",
        "1234",
};
// handle functions
static void handleGetDescriptor(usbip_stage2_header *header);

////TODO: may be ok
void handleUSBControlRequest(usbip_stage2_header *header)
{
    // Table 9-3. Standard Device Requests

    switch (header->u.cmd_submit.request.bmRequestType)
    {
    case 0x00: // ignore..
        switch (header->u.cmd_submit.request.bRequest)
        {
        case USB_REQ_CLEAR_FEATURE:
            ESP_LOGI(TAG,"* CLEAR FEATURE\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_SET_FEATURE:
            ESP_LOGI(TAG,"* SET FEATURE\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_SET_ADDRESS:
            ESP_LOGI(TAG,"* SET ADDRESS\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_SET_DESCRIPTOR:
            ESP_LOGI(TAG,"* SET DESCRIPTOR\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_SET_CONFIGURATION:
            ESP_LOGI(TAG,"* SET CONFIGURATION\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        default:
            ESP_LOGI(TAG,"USB unknown request, bmRequestType:%d,bRequest:%d\r\n",
                      header->u.cmd_submit.request.bmRequestType, header->u.cmd_submit.request.bRequest);
            break;
        }
        break;
    case 0x01: // ignore...
        switch (header->u.cmd_submit.request.bRequest)
        {
        case USB_REQ_CLEAR_FEATURE:
            ESP_LOGI(TAG,"* CLEAR FEATURE\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_SET_FEATURE:
            ESP_LOGI(TAG,"* SET FEATURE\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_SET_INTERFACE:
            ESP_LOGI(TAG,"* SET INTERFACE\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;

        default:
            ESP_LOGI(TAG,"USB unknown request, bmRequestType:%d,bRequest:%d\r\n",
                      header->u.cmd_submit.request.bmRequestType, header->u.cmd_submit.request.bRequest);
            break;
        }
        break;
    case 0x02: // ignore..
        switch (header->u.cmd_submit.request.bRequest)
        {
        case USB_REQ_CLEAR_FEATURE:
            ESP_LOGI(TAG,"* CLEAR FEATURE\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_SET_FEATURE:
            ESP_LOGI(TAG,"* SET INTERFACE\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;

        default:
            ESP_LOGI(TAG,"USB unknown request, bmRequestType:%d,bRequest:%d\r\n",
                      header->u.cmd_submit.request.bmRequestType, header->u.cmd_submit.request.bRequest);
            break;
        }
        break;

    case 0x80: // *IMPORTANT*
#if (USE_WINUSB == 0)
    case 0x81:
#endif
    {
        switch (header->u.cmd_submit.request.bRequest)
        {
        case USB_REQ_GET_CONFIGURATION:
            ESP_LOGI(TAG,"* GET CONIFGTRATION\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_GET_DESCRIPTOR:
            handleGetDescriptor(header); ////TODO: device_qualifier
            break;
        case USB_REQ_GET_STATUS:
            ESP_LOGI(TAG,"* GET STATUS\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        default:
            ESP_LOGI(TAG,"USB unknown request, bmRequestType:%d,bRequest:%d\r\n",
                      header->u.cmd_submit.request.bmRequestType, header->u.cmd_submit.request.bRequest);
            break;
        }
        break;
    }
#if (USE_WINUSB == 1)
    case 0x81: // ignore...
        switch (header->u.cmd_submit.request.bRequest)
        {
        case USB_REQ_GET_INTERFACE:
            ESP_LOGI(TAG,"* GET INTERFACE\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_SET_SYNCH_FRAME:
            ESP_LOGI(TAG,"* SET SYNCH FRAME\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;
        case USB_REQ_GET_STATUS:
            ESP_LOGI(TAG,"* GET STATUS\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;

        default:
            ESP_LOGI(TAG,"USB unknown request, bmRequestType:%d,bRequest:%d\r\n",
                      header->u.cmd_submit.request.bmRequestType, header->u.cmd_submit.request.bRequest);
            break;
        }
        break;
#endif
    case 0x82: // ignore...
        switch (header->u.cmd_submit.request.bRequest)
        {
        case USB_REQ_GET_STATUS:
            ESP_LOGI(TAG,"* GET STATUS\r\n");
            send_stage2_submit_data(header, 0, 0, 0);
            break;

        default:
            ESP_LOGI(TAG,"USB unknown request, bmRequestType:%d,bRequest:%d\r\n",
                      header->u.cmd_submit.request.bmRequestType, header->u.cmd_submit.request.bRequest);
            break;
        }
        break;
    case 0xC0: // Microsoft OS 2.0 vendor-specific descriptor
    {
        uint16_t *wIndex = (uint16_t *)(&(header->u.cmd_submit.request.wIndex));
        switch (*wIndex)
        {
        case MS_OS_20_DESCRIPTOR_INDEX:
            ESP_LOGI(TAG,"* GET MSOS 2.0 vendor-specific descriptor\r\n");
            send_stage2_submit_data(header, 0, msOs20DescriptorSetHeader, sizeof(msOs20DescriptorSetHeader));
            break;
        case MS_OS_20_SET_ALT_ENUMERATION:
            // set alternate enumeration command
            // bAltEnumCode set to 0
            ESP_LOGI(TAG,"Set alternate enumeration.This should not happen.\r\n");
            break;

        default:
            ESP_LOGI(TAG,"USB unknown request, bmRequestType:%d,bRequest:%d,wIndex:%d\r\n",
                      header->u.cmd_submit.request.bmRequestType, header->u.cmd_submit.request.bRequest, *wIndex);
            break;
        }
        break;
    }
    case 0x21: // Set_Idle for HID
        switch (header->u.cmd_submit.request.bRequest)
        {
        case USB_REQ_SET_IDLE:
            ESP_LOGI(TAG,"* SET IDLE\r\n");
            send_stage2_submit(header, 0, 0);
            break;

        default:
            ESP_LOGI(TAG,"USB unknown request, bmRequestType:%d,bRequest:%d\r\n",
                      header->u.cmd_submit.request.bmRequestType, header->u.cmd_submit.request.bRequest);
            break;
        }
        break;
    default:
        ESP_LOGI(TAG,"USB unknown request, bmRequestType:%d,bRequest:%d\r\n",
                  header->u.cmd_submit.request.bmRequestType, header->u.cmd_submit.request.bRequest);
        break;
    }
}

static void handleGetDescriptor(usbip_stage2_header *header)
{
    // 9.4.3 Get Descriptor
    switch (header->u.cmd_submit.request.wValue.u8hi)
    {
    case USB_DT_DEVICE: // get device descriptor
        ESP_LOGI(TAG,"* GET 0x01 DEVICE DESCRIPTOR\r\n");
        send_stage2_submit_data(header, 0, &kUSBd0DeviceDescriptor[0], sizeof(kUSBd0DeviceDescriptor));
        break;

    case USB_DT_CONFIGURATION: // get configuration descriptor
        ESP_LOGI(TAG,"* GET 0x02 CONFIGURATION DESCRIPTOR\r\n");
        ////TODO: ?
        if (header->u.cmd_submit.data_length == USB_DT_CONFIGURATION_SIZE)
        {
            ESP_LOGI(TAG,"Sending only first part of CONFIG\r\n");

            send_stage2_submit(header, 0, header->u.cmd_submit.data_length);
            usbip_network_send(kSock, kUSBd0ConfigDescriptor, sizeof(kUSBd0ConfigDescriptor), 0);
        }
        else
        {
            ESP_LOGI(TAG,"Sending ALL CONFIG\r\n");
            send_stage2_submit(header, 0, sizeof(kUSBd0ConfigDescriptor) + sizeof(kUSBd0InterfaceDescriptor));
            usbip_network_send(kSock, kUSBd0ConfigDescriptor, sizeof(kUSBd0ConfigDescriptor), 0);
            usbip_network_send(kSock, kUSBd0InterfaceDescriptor, sizeof(kUSBd0InterfaceDescriptor), 0);
        }
        break;

    case USB_DT_STRING:
        //ESP_LOGI(TAG,"* GET 0x03 STRING DESCRIPTOR\r\n");

        if (header->u.cmd_submit.request.wValue.u8lo == 0)
        {
            ESP_LOGI(TAG,"** REQUESTED list of supported languages\r\n");
            send_stage2_submit_data(header, 0, kLangDescriptor, sizeof(kLangDescriptor));
        }
        else if (header->u.cmd_submit.request.wValue.u8lo != 0xee)
        {
            //ESP_LOGI(TAG,"low bit : %d\r\n", (int)header->u.cmd_submit.request.wValue.u8lo);
            //ESP_LOGI(TAG,"high bit : %d\r\n", (int)header->u.cmd_submit.request.wValue.u8hi);
            int slen = strlen(strings_list[header->u.cmd_submit.request.wValue.u8lo]);
            int wslen = slen * 2;
            int buff_len = sizeof(usb_string_descriptor) + wslen;
            char temp_buff[64];
            usb_string_descriptor *desc = (usb_string_descriptor *)temp_buff;
            desc->bLength = buff_len;
            desc->bDescriptorType = USB_DT_STRING;
            for (int i = 0; i < slen; i++)
            {
                desc->wData[i] = strings_list[header->u.cmd_submit.request.wValue.u8lo][i];

            }
            send_stage2_submit_data(header, 0, (uint8_t *)temp_buff, buff_len);
        }
        else
        {
            ESP_LOGI(TAG,"low bit : %d\r\n", (int)header->u.cmd_submit.request.wValue.u8lo);
            ESP_LOGI(TAG,"high bit : %d\r\n", (int)header->u.cmd_submit.request.wValue.u8hi);
            ESP_LOGI(TAG,"***Unsupported String descriptor***\r\n");
            send_stage2_submit(header, 0, 0);
        }
        break;

    case USB_DT_INTERFACE:
        ESP_LOGI(TAG,"* GET 0x04 INTERFACE DESCRIPTOR (UNIMPLEMENTED)\r\n");
        ////TODO:UNIMPLEMENTED
        send_stage2_submit(header, 0, 0);
        break;

    case USB_DT_ENDPOINT:
        ESP_LOGI(TAG,"* GET 0x05 ENDPOINT DESCRIPTOR (UNIMPLEMENTED)\r\n");
        ////TODO:UNIMPLEMENTED
        send_stage2_submit(header, 0, 0);
        break;

    case USB_DT_DEVICE_QUALIFIER:
        ESP_LOGI(TAG,"* GET 0x06 DEVICE QUALIFIER DESCRIPTOR\r\n");

        usb_device_qualifier_descriptor desc;

        memset(&desc, 0, sizeof(usb_device_qualifier_descriptor));

        send_stage2_submit_data(header, 0, &desc, sizeof(usb_device_qualifier_descriptor));
        break;

    case USB_DT_OTHER_SPEED_CONFIGURATION:
        ESP_LOGI(TAG,"GET 0x07 [UNIMPLEMENTED] USB_DT_OTHER_SPEED_CONFIGURATION\r\n");
        ////TODO:UNIMPLEMENTED
        send_stage2_submit(header, 0, 0);
        break;

    case USB_DT_INTERFACE_POWER:
        ESP_LOGI(TAG,"GET 0x08 [UNIMPLEMENTED] USB_DT_INTERFACE_POWER\r\n");
        ////TODO:UNIMPLEMENTED
        send_stage2_submit(header, 0, 0);
        break;
#if (USE_WINUSB == 1)
    case USB_DT_BOS:
        ESP_LOGI(TAG,"* GET 0x0F BOS DESCRIPTOR\r\n");
        uint32_t bos_len = header->u.cmd_submit.request.wLength.u8lo | ((uint32_t) header->u.cmd_submit.request.wLength.u8hi << 8);
        bos_len = (sizeof(bosDescriptor) < bos_len) ? sizeof(bosDescriptor) : bos_len;
        send_stage2_submit_data(header, 0, bosDescriptor, bos_len);
        break;
#else
    case USB_DT_HID_REPORT:
        ESP_LOGI(TAG,"* GET 0x22 HID REPORT DESCRIPTOR\r\n");
        send_stage2_submit_data(header, 0, (void *)kHidReportDescriptor, sizeof(kHidReportDescriptor));
        break;
#endif
    default:
        //// TODO: ms os 1.0 descriptor
        ESP_LOGI(TAG,"USB unknown Get Descriptor requested:%d\r\n", header->u.cmd_submit.request.wValue.u8lo);
        ESP_LOGI(TAG,"low bit :%d\r\n",header->u.cmd_submit.request.wValue.u8lo);
        ESP_LOGI(TAG,"high bit :%d\r\n",header->u.cmd_submit.request.wValue.u8hi);
        break;
    }
}
