/***************************************************************************//**
 * @file
 * @brief HAL config initialization
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include PLATFORM_HEADER
#include "hal/hal.h"
#include "em_device.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "ecode.h"
#include "plugin/antenna/antenna.h"
#include "plugin/serial/com.h"
#include "plugin/serial/com_config.h"
#if (PHY_RAIL || PHY_DUALRAIL)
#include "rail.h"
#include "rail_ieee802154.h"
#elif defined (_EFR_DEVICE)
#include "rfhal_pti.h"
#endif
#if HAL_IOEXP_VCOM_ENABLE
#include "bsp.h"
#include "retargetserial.h"
#endif
#if HAL_FEM_ENABLE
#include "util/plugin/plugin-common/fem-control/fem-control.h"
#endif
#if HAL_COEX_ENABLE
#include "coexistence/protocol/ieee802154/coexistence-802154.h"
#endif

static void halConfigClockInit(void)
{
// Initialize HFXO if used
#if ((HAL_CLK_HFCLK_SOURCE == HAL_CLK_HFCLK_SOURCE_HFXO)   \
  || ((HAL_CLK_HFCLK_SOURCE == HAL_CLK_HFCLK_SOURCE_HFRCO) \
  && (HAL_CLK_PLL_CONFIGURATION == HAL_CLK_PLL_CONFIGURATION_80MHZ)))
  #if !BSP_CLK_HFXO_PRESENT
    #error Cannot select HFXO when HFXO is not present
  #endif

  CMU_HFXOInit_TypeDef hfxoInit = BSP_CLK_HFXO_INIT;

  #if defined(BSP_CLK_HFXO_CTUNE) && (BSP_CLK_HFXO_CTUNE >= 0)
  #if defined(_SILICON_LABS_32B_SERIES_2)
  hfxoInit.ctuneXiAna = BSP_CLK_HFXO_CTUNE;
  hfxoInit.ctuneXoAna = BSP_CLK_HFXO_CTUNE;
  #else
  hfxoInit.ctuneSteadyState = BSP_CLK_HFXO_CTUNE;
  #endif // series 2
  #endif // BSP_CLK_HFXO_CTUNE

  #if defined(_SILICON_LABS_32B_SERIES_2)
  uint8_t customSteadyCtuneXi;
  uint8_t customSteadyCtuneXo;
  uint16_t customSteadyCtuneToken;

  // check factory calibrated ctune value
  if (halInternalGetModuleCtuneXiXo(&customSteadyCtuneXi, &customSteadyCtuneXo)) {
    hfxoInit.ctuneXiAna = customSteadyCtuneXi;
    hfxoInit.ctuneXoAna = customSteadyCtuneXo;
  }
  // otherwise check for ctune in mfg token
  else if (halInternalGetCtuneToken(&customSteadyCtuneToken, &customSteadyCtuneToken)) {
    hfxoInit.ctuneXiAna = (uint8_t)customSteadyCtuneToken;
    hfxoInit.ctuneXoAna = (uint8_t)customSteadyCtuneToken;
  }
  #else
  uint16_t customSteadyCtuneModule;
  uint16_t customSteadyCtuneToken;
  // check factory calibrated ctune value
  if (halInternalGetModuleCtune(&customSteadyCtuneModule)) {
    hfxoInit.ctuneSteadyState = customSteadyCtuneModule;
  }
  // otherwise check for ctune in mfg token
  else if (halInternalGetCtuneToken(&customSteadyCtuneToken, &customSteadyCtuneToken)) {
    hfxoInit.ctuneSteadyState = customSteadyCtuneToken;
  }
  #endif // series 2
  CMU_HFXOInit(&hfxoInit);
#endif // hfxo or hfrco with hfxo

#if (HAL_CLK_HFCLK_SOURCE == HAL_CLK_HFCLK_SOURCE_HFXO)
  // Setting system HFXO frequency
  SystemHFXOClockSet(BSP_CLK_HFXO_FREQ);

  // Select the HFXO and, if supported, manually turn off HFRCO
  // If requested and supported, turn on autostart/select for series 1 devices
  #if defined(_SILICON_LABS_32B_SERIES_1)
    #if defined(HAL_CLK_HFXO_AUTOSTART) && HAL_CLK_HFXO_AUTOSTART == HAL_CLK_HFXO_AUTOSTART_SELECT
  // Automatically start and select HFXO
  CMU_HFXOAutostartEnable(0, true, true);
    #else
      #if defined(HAL_CLK_HFXO_AUTOSTART) && HAL_CLK_HFXO_AUTOSTART == HAL_CLK_HFXO_AUTOSTART_START
  // Automatically start HFXO
  CMU_HFXOAutostartEnable(0, true, false);
      #endif // HAL_CLK_HFXO_AUTOSTART
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
    #endif // HAL_CLK_HFXO_AUTOSTART

  /* HFRCO not needed when using HFXO */
  CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
  #elif defined(_SILICON_LABS_32B_SERIES_2) // defined(_SILICON_LABS_32B_SERIES_1)
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFXO);
  #else // defined(_SILICON_LABS_32B_SERIES_2)
    #error Unknown device series
  #endif // defined(_SILICON_LABS_32B_SERIES_2)

#elif (HAL_CLK_HFCLK_SOURCE == HAL_CLK_HFCLK_SOURCE_HFRCO)
  #if defined(_SILICON_LABS_32B_SERIES_2)
  // Use LFXO at 40MHz
    #if (HAL_CLK_PLL_CONFIGURATION == HAL_CLK_PLL_CONFIGURATION_40MHZ)
  CMU_LFXOInit_TypeDef lfxoInit = CMU_LFXOINIT_DEFAULT;
    #if defined(BSP_CLK_LFXO_CTUNE) && BSP_CLK_LFXO_CTUNE > 0
  lfxoInit.ctune = BSP_CLK_LFXO_CTUNE;
    #endif
  CMU_LFXOInit(&lfxoInit);
  CMU_DPLLInit_TypeDef dpllInit = CMU_DPLL_LFXO_TO_40MHZ;
    #endif
  // Use HFXO at 80MHz
    #if (HAL_CLK_PLL_CONFIGURATION == HAL_CLK_PLL_CONFIGURATION_80MHZ)
  CMU_DPLLInit_TypeDef dpllInit = CMU_DPLL_HFXO_TO_80MHZ;
    #endif
    #if (HAL_CLK_PLL_CONFIGURATION != HAL_CLK_PLL_CONFIGURATION_NONE)
  bool locked = false;
  while (!locked) {
    locked = CMU_DPLLLock(&dpllInit);
  }
    #endif
  CMU_ClockSelectSet(cmuClock_SYSCLK, cmuSelect_HFRCODPLL);
  #else
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
  #endif // series 2
#else
  #error Must define HAL_CLK_HFCLK_SOURCE
#endif // HAL_CLK_HFCLK_SOURCE

/* LFCLK */
#if (HAL_CLK_LFACLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO)  \
  || (HAL_CLK_LFBCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO) \
  || (HAL_CLK_LFCCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO) \
  || (HAL_CLK_LFECLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO)
  #if !BSP_CLK_LFXO_PRESENT
    #error "Cannot select LFXO when LFXO is not present"
  #else
  CMU_LFXOInit_TypeDef lfxoInit = BSP_CLK_LFXO_INIT;

  #if defined(BSP_CLK_LFXO_CTUNE) && BSP_CLK_LFXO_CTUNE > 0
  lfxoInit.ctune = BSP_CLK_LFXO_CTUNE;
  #endif
  CMU_LFXOInit(&lfxoInit);

  // Set system LFXO frequency
  SystemLFXOClockSet(BSP_CLK_LFXO_FREQ);
  #endif
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
  // LFA
#if (HAL_CLK_LFACLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO)
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
#elif (HAL_CLK_LFACLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFRCO)
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);
#elif (HAL_CLK_LFACLK_SOURCE == HAL_CLK_LFCLK_SOURCE_ULFRCO)
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
#elif (HAL_CLK_LFACLK_SOURCE == HAL_CLK_LFCLK_SOURCE_HFLE)
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_HFCLKLE);
#endif

  // LFB
#if (HAL_CLK_LFBCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO)
  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
#elif (HAL_CLK_LFBCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFRCO)
  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFRCO);
#elif (HAL_CLK_LFBCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_ULFRCO)
  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_ULFRCO);
#elif (HAL_CLK_LFBCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_HFLE)
  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_HFCLKLE);
#endif

  // LFC
#if (HAL_CLK_LFCCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO)
  CMU_ClockSelectSet(cmuClock_LFC, cmuSelect_LFXO);
#elif (HAL_CLK_LFCCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFRCO)
  CMU_ClockSelectSet(cmuClock_LFC, cmuSelect_LFRCO);
#endif

  // LFE
#if (HAL_CLK_LFECLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO)
  CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFXO);
#elif (HAL_CLK_LFECLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFRCO)
  CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFRCO);
#elif (HAL_CLK_LFECLK_SOURCE == HAL_CLK_LFCLK_SOURCE_ULFRCO)
  CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_ULFRCO);
#endif

#elif defined(_SILICON_LABS_32B_SERIES_2)
  // EM23
#if (HAL_CLK_EM23CLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO)
  CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_LFXO);
#elif (HAL_CLK_EM23CLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFRCO)
  CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_LFRCO);
#elif (HAL_CLK_EM23CLK_SOURCE == HAL_CLK_LFCLK_SOURCE_ULFRCO)
  CMU_ClockSelectSet(cmuClock_EM23GRPACLK, cmuSelect_ULFRCO);
#endif

  // EM4
#if (HAL_CLK_EM4CLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO)
  CMU_ClockSelectSet(cmuClock_EM4GRPACLK, cmuSelect_LFXO);
#elif (HAL_CLK_EM4CLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFRCO)
  CMU_ClockSelectSet(cmuClock_EM4GRPACLK, cmuSelect_LFRCO);
#elif (HAL_CLK_EM4CLK_SOURCE == HAL_CLK_LFCLK_SOURCE_ULFRCO)
  CMU_ClockSelectSet(cmuClock_EM4GRPACLK, cmuSelect_ULFRCO);
#endif

  // RTCC
#if (HAL_CLK_RTCCCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFXO)
  CMU_ClockSelectSet(cmuClock_RTCCCLK, cmuSelect_LFXO);
#elif (HAL_CLK_RTCCCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_LFRCO)
  CMU_ClockSelectSet(cmuClock_RTCCCLK, cmuSelect_LFRCO);
#elif (HAL_CLK_RTCCCLK_SOURCE == HAL_CLK_LFCLK_SOURCE_ULFRCO)
  CMU_ClockSelectSet(cmuClock_RTCCCLK, cmuSelect_ULFRCO);
#endif
#endif // _SILICON_LABS_32B_SERIES_1
}

Ecode_t halConfigInit(void)
{
  Ecode_t status = ECODE_OK;

  INTERRUPTS_ON();   //enables using USB and the proper emberSerialReadByte
#if defined (_EMU_DCDCCTRL_MASK)
#if BSP_DCDC_PRESENT
  EMU_DCDCInit_TypeDef dcdcInit = BSP_DCDC_INIT;
  #if HAL_DCDC_BYPASS
  dcdcInit.dcdcMode = emuDcdcMode_Bypass;
  #endif
  EMU_DCDCInit(&dcdcInit);
#else
  EMU_DCDCPowerOff();
#endif
#endif //_EMU_DCDCCTRL_MASK

#if HAL_EMU_ENABLE
  EMU_EM23Init_TypeDef em23init = EMU_EM23INIT_DEFAULT;
  #if HAL_EMU_EM23_VREG
  em23init.em23VregFullEn = true;
  #endif
  #if HAL_EMU_EM23_VSCALE == HAL_EMU_EM23_VSCALE_FASTWAKEUP
  em23init.vScaleEM23Voltage = emuVScaleEM23_FastWakeup;
  #elif HAL_EMU_EM23_VSCALE == HAL_EMU_EM23_VSCALE_LOWPOWER
  em23init.vScaleEM23Voltage = emuVScaleEM23_LowPower;
  #endif
  EMU_EM23Init(&em23init);
#endif //HAL_EMU_ENABLE

  halConfigClockInit();

#if (HAL_BUTTON_COUNT > 0)
  halInternalInitButton();
#endif

#if (HAL_LED_COUNT > 0)
  halInternalInitLed();
#endif

#if (HAL_GPIO_MAX > 0)
  halConfigInitGpio();
#endif

#if defined (_EFR_DEVICE)
#if (HAL_PTI_ENABLE)
  #if HAL_PTI_MODE == HAL_PTI_MODE_SPI
  // SPI Mode
    #define PTI_MODE RAIL_PTI_MODE_SPI
  #elif HAL_PTI_MODE == HAL_PTI_MODE_UART
  // UART Mode
    #define PTI_MODE RAIL_PTI_MODE_UART
  // DCLK unused in UART mode
    #undef BSP_PTI_DCLK_LOC
    #undef BSP_PTI_DCLK_PORT
    #undef BSP_PTI_DCLK_PIN
    #define BSP_PTI_DCLK_LOC  0
    #define BSP_PTI_DCLK_PORT 0
    #define BSP_PTI_DCLK_PIN  0
  #elif HAL_PTI_MODE == HAL_PTI_MODE_UART_ONEWIRE
  // Onewire UART Mode
    #define PTI_MODE RAIL_PTI_MODE_UART_ONEWIRE
  // DCLK unused in onewire mode
    #undef BSP_PTI_DCLK_LOC
    #undef BSP_PTI_DCLK_PORT
    #undef BSP_PTI_DCLK_PIN
    #define BSP_PTI_DCLK_LOC    0
    #define BSP_PTI_DCLK_PORT   0
    #define BSP_PTI_DCLK_PIN    0
  // DFRAME unused in onewire mode
    #undef BSP_PTI_DFRAME_LOC
    #undef BSP_PTI_DFRAME_PORT
    #undef BSP_PTI_DFRAME_PIN
    #define BSP_PTI_DFRAME_LOC  0
    #define BSP_PTI_DFRAME_PORT 0
    #define BSP_PTI_DFRAME_PIN  0
  #else
    #error HAL_PTI_MODE not recognized
  #endif //HAL_PTI_MODE
  #if !defined(BSP_PTI_DFRAME_LOC)
    #define BSP_PTI_DFRAME_LOC 0
  #endif
  #if !defined(BSP_PTI_DOUT_LOC)
    #define BSP_PTI_DOUT_LOC 0
  #endif
  #if !defined(BSP_PTI_DCLK_LOC)
    #define BSP_PTI_DCLK_LOC 0
  #endif
  RAIL_PtiConfig_t ptiInit = { PTI_MODE,
                               HAL_PTI_BAUD_RATE,                       /* 1.6 MHz baud */
                               BSP_PTI_DOUT_LOC,                        /* DOUT location */
                               (GPIO_Port_TypeDef) BSP_PTI_DOUT_PORT,   /* DOUT port */
                               BSP_PTI_DOUT_PIN,                        /* DOUT pin */
                               BSP_PTI_DCLK_LOC,                        /* DCLK location */
                               (GPIO_Port_TypeDef) BSP_PTI_DCLK_PORT,   /* DCLK port */
                               BSP_PTI_DCLK_PIN,                        /* DCLK pin */
                               BSP_PTI_DFRAME_LOC,                      /* DFRAME location */
                               (GPIO_Port_TypeDef) BSP_PTI_DFRAME_PORT, /* DFRAME port */
                               BSP_PTI_DFRAME_PIN };                    /* DFRAME pin */
#if (PHY_RAIL || PHY_DUALRAIL)
  RAIL_ConfigPti(NULL, &ptiInit);
#else
  PTI_Config(&ptiInit);
#endif
#endif // HAL_PTI_ENABLE
#endif // defined (_EFR_DEVICE)

#if (HAL_COEX_ENABLE)
#if     (PHY_RAIL || PHY_DUALRAIL)
  (void) RAIL_ConfigMultiTimer(true);
#endif//(PHY_RAIL || PHY_DUALRAIL)
  HalPtaOptions ptaOptions = halPtaGetOptions();
  halPtaSetOptions(ptaOptions);
  halInternalInitPta();
#endif

#if (HAL_ANTDIV_ENABLE)
  halInternalInitAntennaDiversity();
#endif

#if (HAL_SERIAL_USART0_ENABLE)
  COM_Init_t initDataUsart0 = COM_USART0_DEFAULT;
  status = COM_Init(comPortUsart0, &initDataUsart0);
  if (status != ECODE_OK) {
    return status;
  }
#endif
#if (HAL_SERIAL_USART1_ENABLE)
  COM_Init_t initDataUsart1 = (COM_Init_t) COM_USART1_DEFAULT;
  status = COM_Init(comPortUsart1, &initDataUsart1);
  if (status != ECODE_OK) {
    return status;
  }
#endif
#if (HAL_SERIAL_USART2_ENABLE)
  COM_Init_t initDataUsart2 = (COM_Init_t) COM_USART2_DEFAULT;
  status = COM_Init(comPortUsart2, &initDataUsart2);
  if (status != ECODE_OK) {
    return status;
  }
#endif
#if (HAL_SERIAL_USART3_ENABLE)
  COM_Init_t initDataUsart3 = (COM_Init_t) COM_USART3_DEFAULT;
  status = COM_Init(comPortUsart3, &initDataUsart3);
  if (status != ECODE_OK) {
    return status;
  }
#endif
#if (HAL_SERIAL_LEUART0_ENABLE)
  COM_Init_t initDataLeuart0 = (COM_Init_t) COM_LEUART0_DEFAULT;
  status = COM_Init(comPortLeuart0, &initDataLeuart0);
  if (status != ECODE_OK) {
    return status;
  }
#endif
#if (HAL_SERIAL_LEUART1_ENABLE)
  COM_Init_t initDataLeuart1 = (COM_Init_t) COM_LEUART1_DEFAULT;
  status = COM_Init(comPortLeuart1, &initDataLeuart1);
  if (status != ECODE_OK) {
    return status;
  }
#endif

#if (HAL_VCOM_ENABLE)
  #if HAL_IOEXP_VCOM_ENABLE
  BSP_Init(BSP_INIT_IOEXP);
  BSP_PeripheralAccess(BSP_IOEXP_VCOM, true);
  #elif defined (BSP_VCOM_ENABLE_PORT)
  GPIO_PinModeSet(BSP_VCOM_ENABLE_PORT, BSP_VCOM_ENABLE_PIN, gpioModePushPull, 1);
  #else
  #warning VCOM enabled without configuring enable pin or IOEXP
  #endif
#endif // (HAL_SERIAL_VCOM_ENABLE)

#if (HAL_SERIAL_RXWAKE_ENABLE)
  COM_RxGpioWakeInit();
#endif

#if (HAL_FEM_ENABLE)
  initFem();
#endif

#if (HAL_WDOG_ENABLE)
  halInternalEnableWatchDog();
#endif
  return status;
}

#if ANTENNA_USE_RAIL_SCHEME && (ANTENNA_RX_DEFAULT_MODE != HAL_ANTENNA_MODE_DISABLED)

#if HAL_COEX_PHY_ENABLED
#define config2p4GHzRadioAntDiv(railHandle) \
  (RAIL_IEEE802154_Config2p4GHzRadioAntDivCoex(railHandle))
#else //!HAL_COEX_PHY_ENABLED
#define config2p4GHzRadioAntDiv(railHandle) \
  (RAIL_IEEE802154_Config2p4GHzRadioAntDiv(railHandle))
#endif //HAL_COEX_PHY_ENABLED

extern EmberStatus emRadioConfigRxAntenna(HalAntennaMode mode);

RAIL_Status_t halPluginConfig2p4GHzRadio(RAIL_Handle_t railHandle)
{
  // Establish the proper radio config supporting antenna diversity
  assert(config2p4GHzRadioAntDiv(railHandle)
         == RAIL_STATUS_NO_ERROR);

  assert(halAntennaConfigRailAntenna(railHandle) == RAIL_STATUS_NO_ERROR);

  // Tell RAIL what Rx antenna mode to use
  assert(emRadioConfigRxAntenna(halGetAntennaRxMode()) == EMBER_SUCCESS);

  return RAIL_STATUS_NO_ERROR;
}

#elif HAL_COEX_PHY_ENABLED //!(ANTENNA_USE_RAIL_SCHEME && RX_MODE != DISABLED)
RAIL_Status_t halPluginConfig2p4GHzRadio(RAIL_Handle_t railHandle)
{
  // Establish the proper radio config supporting antenna diversity
  assert(RAIL_IEEE802154_Config2p4GHzRadioCoex(railHandle)
         == RAIL_STATUS_NO_ERROR);
  return RAIL_STATUS_NO_ERROR;
}
#else //(!HAL_COEX_PHY_ENABLED && !(ANTENNA_USE_RAIL_SCHEME && RX_MODE != DISABLED))
// No halPluginConfig2p4GHzRadio() stub needed.
#endif //(HAL_COEX_PHY_ENABLED || ANTENNA_USE_RAIL_SCHEME)
