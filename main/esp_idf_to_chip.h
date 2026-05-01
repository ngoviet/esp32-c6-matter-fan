/**
 * ESP-IDF to CHIP configuration mapping header
 * 
 * This file maps ESP-IDF configuration macros to CHIP configuration macros
 * for use in esp_matter_core.cpp and other ESP-Matter components.
 */

#pragma once

#include <sdkconfig.h>
#include <esp_idf_version.h>

/* CHIP configuration macros from ESP-IDF */
#ifdef CONFIG_CHIP_ENABLE_ARC
#define CHIP_CONFIG_ENABLE_ARC 1
#else
#define CHIP_CONFIG_ENABLE_ARC 0
#endif

#ifdef CONFIG_CHIP_ENABLE_TESTING
#define CHIP_CONFIG_TESTING 1
#else
#define CHIP_CONFIG_TESTING 0
#endif

/* TCP Transport */
#ifdef CONFIG_ENABLE_TCP_TRANSPORT
#define CHIP_CONFIG_ENABLE_TCP_TRANSPORT 1
#else
#define CHIP_CONFIG_ENABLE_TCP_TRANSPORT 0
#endif

/* IPv4 */
#ifdef CONFIG_LWIP_IPV4
#define CHIP_INET_CONFIG_ENABLE_IPV4 1
#else
#define CHIP_INET_CONFIG_ENABLE_IPV4 0
#endif

/* OpenThread */
#ifdef CONFIG_OPENTHREAD_ENABLED
#define CHIP_SYSTEM_CONFIG_USE_OPENTHREAD_ENDPOINT 1
#else
#define CHIP_SYSTEM_CONFIG_USE_OPENTHREAD_ENDPOINT 0
#endif

#ifdef CONFIG_OPENTHREAD_FTD
#define CHIP_SYSTEM_CONFIG_OPENTHREAD_FTD 1
#else
#define CHIP_SYSTEM_CONFIG_OPENTHREAD_FTD 0
#endif

/* LWIP */
#ifdef CONFIG_LWIP_ENABLED
#define CHIP_SYSTEM_CONFIG_USE_LWIP 1
#else
#define CHIP_SYSTEM_CONFIG_USE_LWIP 0
#endif

/* Socket-based networking */
#ifndef CHIP_SYSTEM_CONFIG_USE_LWIP
#define CHIP_SYSTEM_CONFIG_USE_SOCKETS 0
#define CHIP_SYSTEM_CONFIG_USE_NETWORK_FRAMEWORK 0
#endif

/* FreeRTOS locking */
#ifdef CONFIG_FREERTOS_UNICORE
#define CHIP_SYSTEM_CONFIG_FREERTOS_LOCKING_UNICORE 1
#else
#define CHIP_SYSTEM_CONFIG_FREERTOS_LOCKING_UNICORE 0
#endif

/* ACL support */
#ifdef CONFIG_CHIP_ENABLE_ACL_EXTENSIONS
#define CHIP_CONFIG_ENABLE_ACL_EXTENSIONS 1
#else
#define CHIP_CONFIG_ENABLE_ACL_EXTENSIONS 0
#endif

/* ICD Server */
#ifdef CONFIG_CHIP_ENABLE_ICD_SERVER
#define CHIP_CONFIG_ENABLE_ICD_SERVER 1
#else
#define CHIP_CONFIG_ENABLE_ICD_SERVER 0
#endif

/* Pairing Auto-start */
#ifdef CONFIG_CHIP_ENABLE_PAIRING_AUTOSTART
#define CHIP_DEVICE_CONFIG_ENABLE_PAIRING_AUTOSTART 1
#else
#define CHIP_DEVICE_CONFIG_ENABLE_PAIRING_AUTOSTART 0
#endif

/* WiFi provisioning */
#ifdef CONFIG_ESP_WIFI_PROV_ENABLED
#define ESP_WIFI_PROV_ENABLED 1
#else
#define ESP_WIFI_PROV_ENABLED 0
#endif

/* Thread enabled */
#ifdef CONFIG_ESP_MATTER_THREAD_ENABLED
#define ESP_MATTER_THREAD_ENABLED 1
#else
#define ESP_MATTER_THREAD_ENABLED 0
#endif
