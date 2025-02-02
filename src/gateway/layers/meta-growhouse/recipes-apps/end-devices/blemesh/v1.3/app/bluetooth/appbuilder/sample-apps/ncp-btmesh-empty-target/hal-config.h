#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include "board_features.h"
#include "hal-config-board.h"
#include "hal-config-app-common.h"

#define HAL_UARTNCP_BAUD_RATE             (115200)
#define HAL_UARTNCP_FLOW_CONTROL          (HAL_USART_FLOW_CONTROL_HWUART)

#ifndef HAL_VCOM_ENABLE
#define HAL_VCOM_ENABLE                   (1)
#endif
#define HAL_I2CSENSOR_ENABLE              (0)
#define HAL_SPIDISPLAY_ENABLE             (0)

#define NCP_USART_STOPBITS    usartStopbits1  // Number of stop bits
#define NCP_USART_PARITY      usartNoParity   // Parity configuration
#define NCP_USART_OVS         usartOVS16      // Oversampling mode
#define NCP_USART_MVDIS       0               // Majority Vote Disable for 16x, 8x

#if (HAL_UARTNCP_FLOW_CONTROL == HAL_USART_FLOW_CONTROL_HWUART)
  #define NCP_USART_FLOW_CONTROL_TYPE    uartdrvFlowControlHwUart
#else
  #define NCP_USART_FLOW_CONTROL_TYPE    uartdrvFlowControlNone
#endif

// Define if NCP sleep functionality is requested
//#define NCP_DEEP_SLEEP_ENABLED

#if defined(NCP_DEEP_SLEEP_ENABLED)
  #define NCP_WAKEUP_PORT
  #define NCP_WAKEUP_PIN
  #define NCP_WAKEUP_POLARITY
#endif

// Define if NCP wakeup functionality is requested
//#define NCP_HOST_WAKEUP_ENABLED

#if defined(NCP_HOST_WAKEUP_ENABLED)
  #define NCP_HOST_WAKEUP_PORT
  #define NCP_HOST_WAKEUP_PIN
  #define NCP_HOST_WAKEUP_POLARITY
#endif

#endif
