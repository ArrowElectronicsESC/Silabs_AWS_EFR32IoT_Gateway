/***************************************************************************//**
 * @file
 * @brief Provide BSP (board support package) configuration parameters.
 * @version 5.5.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#ifndef SILICON_LABS_BSPCONFIG_H
#define SILICON_LABS_BSPCONFIG_H

#define BSP_STK
#define BSP_STK_BRD2204A

#define BSP_BCC_USART         USART1
#define BSP_BCC_CLK           cmuClock_USART1
#define BSP_BCC_TX_LOCATION   USART_ROUTELOC0_TXLOC_LOC1
#define BSP_BCC_RX_LOCATION   USART_ROUTELOC0_RXLOC_LOC1
#define BSP_BCC_TXPORT        gpioPortD
#define BSP_BCC_TXPIN         0
#define BSP_BCC_RXPORT        gpioPortD
#define BSP_BCC_RXPIN         1
#define BSP_BCC_ENABLE_PORT   gpioPortA
#define BSP_BCC_ENABLE_PIN    8                 /* VCOM_ENABLE */

#define BSP_GPIO_LEDS
#define BSP_NO_OF_LEDS        2
#define BSP_GPIO_LED0_PORT    gpioPortD
#define BSP_GPIO_LED0_PIN     2
#define BSP_GPIO_LED1_PORT    gpioPortC
#define BSP_GPIO_LED1_PIN     2
#define BSP_GPIO_LEDARRAY_INIT { { BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN }, { BSP_GPIO_LED1_PORT, BSP_GPIO_LED1_PIN } }

#define BSP_GPIO_BUTTONS
#define BSP_NO_OF_BUTTONS     2
#define BSP_GPIO_PB0_PORT     gpioPortD
#define BSP_GPIO_PB0_PIN      5
#define BSP_GPIO_PB1_PORT     gpioPortC
#define BSP_GPIO_PB1_PIN      9

#define BSP_GPIO_BUTTONARRAY_INIT { { BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN }, { BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN } }

#define BSP_INIT_DEFAULT  0

#if !defined(EMU_DCDCINIT_STK_DEFAULT)
/* Use emlib defaults */
#define EMU_DCDCINIT_STK_DEFAULT          EMU_DCDCINIT_DEFAULT
#endif

#if !defined(CMU_HFXOINIT_STK_DEFAULT)
#define CMU_HFXOINIT_STK_DEFAULT                   \
  {                                                \
    _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,            \
    0x78,                                          \
    _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_DEFAULT,     \
    _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_DEFAULT, \
    _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_DEFAULT,   \
    _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_DEFAULT,    \
    _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,   \
    cmuOscMode_Crystal,                            \
  }
#endif

#define BSP_BCP_VERSION 2
#include "bsp_bcp.h"

#endif /* SILICON_LABS_BSPCONFIG_H */
