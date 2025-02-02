/***************************************************************************//**
 * @file em_usbd.c
 * @brief USB protocol stack library API for EFM32/EZR32.
 * @version 5.5.0
 ******************************************************************************
 * # License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#include "em_device.h"
#if defined(USB_PRESENT) && (USB_COUNT == 1)
#include "em_usb.h"
#if defined(USB_DEVICE)

#include "em_cmu.h"
#include "em_core.h"
#include "em_system.h"
#include "em_usbtypes.h"
#include "em_usbhal.h"
#include "em_usbd.h"

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

static USBD_Device_TypeDef device;
USBD_Device_TypeDef *dev = &device;

static const char *stateNames[] =
{
  [USBD_STATE_NONE]       = "NONE      ",
  [USBD_STATE_ATTACHED]   = "ATTACHED  ",
  [USBD_STATE_POWERED]    = "POWERED   ",
  [USBD_STATE_DEFAULT]    = "DEFAULT   ",
  [USBD_STATE_ADDRESSED]  = "ADDRESSED ",
  [USBD_STATE_CONFIGURED] = "CONFIGURED",
  [USBD_STATE_SUSPENDED]  = "SUSPENDED ",
  [USBD_STATE_LASTMARKER] = "UNDEFINED "
};

/** @endcond */

/***************************************************************************//**
 * @brief
 *   Abort all pending transfers.
 *
 * @details
 *   Aborts transfers for all endpoints currently in use. Pending
 *   transfers on the default endpoint (EP0) are not aborted.
 ******************************************************************************/
void USBD_AbortAllTransfers(void)
{
  CORE_ATOMIC_SECTION(
    USBDHAL_AbortAllTransfers(USB_STATUS_EP_ABORTED);
    )
}

/***************************************************************************//**
 * @brief
 *   Abort a pending transfer on a specific endpoint.
 *
 * @param[in] epAddr
 *   The address of the endpoint to abort.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code enumerated
 *   in @ref USB_Status_TypeDef.
 ******************************************************************************/
int USBD_AbortTransfer(int epAddr)
{
  USB_XferCompleteCb_TypeDef callback;
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr(epAddr);
  CORE_DECLARE_IRQ_STATE;

  if ( ep == NULL ) {
    DEBUG_USB_API_PUTS("\nUSBD_AbortTransfer(), Illegal endpoint");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ( ep->num == 0 ) {
    DEBUG_USB_API_PUTS("\nUSBD_AbortTransfer(), Illegal endpoint");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  CORE_ENTER_ATOMIC();
  if ( ep->state == D_EP_IDLE ) {
    CORE_EXIT_ATOMIC();
    return USB_STATUS_OK;
  }

  USBD_AbortEp(ep);

  ep->state = D_EP_IDLE;
  if ( ep->xferCompleteCb ) {
    callback = ep->xferCompleteCb;
    ep->xferCompleteCb = NULL;

    if ( (dev->lastState == USBD_STATE_CONFIGURED)
         && (dev->state  == USBD_STATE_ADDRESSED)) {
      USBDHAL_DeactivateEp(ep);
    }

    DEBUG_TRACE_ABORT(USB_STATUS_EP_ABORTED);
    callback(USB_STATUS_EP_ABORTED, ep->xferred, ep->remaining);
  }

  CORE_EXIT_ATOMIC();
  return USB_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   Start USB device operation.
 *
 * @details
 *   Device operation is started by connecting a pullup resistor on the
 *   appropriate USB data line.
 ******************************************************************************/
void USBD_Connect(void)
{
  CORE_ATOMIC_SECTION(
    USBDHAL_Connect();
    )
}

/***************************************************************************//**
 * @brief
 *   Stop USB device operation.
 *
 * @details
 *   Device operation is stopped by disconnecting the pullup resistor from the
 *   appropriate USB data line. Often referred to as a "soft" disconnect.
 ******************************************************************************/
void USBD_Disconnect(void)
{
  CORE_ATOMIC_SECTION(
    USBDHAL_Disconnect();
    )
}

/***************************************************************************//**
 * @brief
 *   Check if an endpoint is busy doing a transfer.
 *
 * @param[in] epAddr
 *   The address of the endpoint to check.
 *
 * @return
 *   True if endpoint is busy, false otherwise.
 ******************************************************************************/
bool USBD_EpIsBusy(int epAddr)
{
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr(epAddr);

  if ( ep == NULL ) {
    DEBUG_USB_API_PUTS("\nUSBD_EpIsBusy(), Illegal endpoint");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ( ep->state == D_EP_IDLE ) {
    return false;
  }

  return true;
}

/***************************************************************************//**
 * @brief
 *   Get current USB device state.
 *
 * @return
 *   Device USB state. See @ref USBD_State_TypeDef.
 ******************************************************************************/
USBD_State_TypeDef USBD_GetUsbState(void)
{
  return dev->state;
}

/***************************************************************************//**
 * @brief
 *   Get a string naming a device USB state.
 *
 * @param[in] state
 *   Device USB state. See @ref USBD_State_TypeDef.
 *
 * @return
 *   State name string pointer.
 ******************************************************************************/
const char *USBD_GetUsbStateName(USBD_State_TypeDef state)
{
  if ( state > USBD_STATE_LASTMARKER ) {
    state = USBD_STATE_LASTMARKER;
  }

  return stateNames[state];
}

/***************************************************************************//**
 * @brief
 *   Initializes USB device hardware and internal protocol stack data structures,
 *   then connects the data-line (D+ or D-) pullup resistor to signal host that
 *   enumeration can begin.
 *
 * @note
 *   You may later use @ref USBD_Disconnect() and @ref USBD_Connect() to force
 *   reenumeration.
 *
 * @param[in] p
 *   Pointer to device initialization struct. See @ref USBD_Init_TypeDef.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code enumerated
 *   in @ref USB_Status_TypeDef.
 ******************************************************************************/
int USBD_Init(const USBD_Init_TypeDef *p)
{
  int numEps;
  USBD_Ep_TypeDef *ep;
  uint8_t txFifoNum;
  uint8_t *conf, *confEnd;
#if defined(_EFM32_HAPPY_FAMILY)
  SYSTEM_ChipRevision_TypeDef chipRev;
#endif
  USB_EndpointDescriptor_TypeDef *epd;
  USB_InterfaceDescriptor_TypeDef *id;
  uint32_t totalRxFifoSize, totalTxFifoSize, numInEps, numOutEps;
  CORE_DECLARE_IRQ_STATE;

// ---------- Enable oscillators. ----------------------------------------------
#if (USB_PWRSAVE_MODE)
  /* Select a clock source for clocking USB peripheral in "power-down" mode. */
  #if (USB_USBC_32kHz_CLK == USB_USBC_32kHz_CLK_LFXO)
  CMU_OscillatorEnable(cmuOsc_LFXO, true, false);
  #else
  CMU_OscillatorEnable(cmuOsc_LFRCO, true, false);
  #endif
#endif

#if !defined(USB_PRESENT)
#error "No USB support on this device."

#elif defined(_SILICON_LABS_32B_SERIES_0) && !defined(_EFM32_HAPPY_FAMILY)
// ------- Series 0, Giant, Leopard and Wonder Geckos --------------------------

  /* HFXO is the only possible core/usb oscillator. */
  if (CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_HFXO) {
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  }

#elif defined(_EFM32_HAPPY_FAMILY)
// ------- Series 0, Happy Gecko -----------------------------------------------

  /* Happy Gecko can use HFRCO or HFXO but not USHFRCO as core clock. */
  /* (This restriction applies if using EM2 energy mode).             */
  /* USHFRCO is the only possible usb oscillator.                     */

  #if !defined(USB_CORECLK_HFRCO)
  if ( CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_HFXO ) {
    CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  }
  #endif

  /* LFC clock is needed to detect USB suspend when LEMIDLE is activated. */
  CMU_ClockEnable(cmuClock_CORELE, true);
  #if (USB_USBC_32kHz_CLK == USB_USBC_32kHz_CLK_LFXO) \
  || (USB_USBLEM_CLK == USB_USBLEM_CLK_LFXO)
  CMU_ClockSelectSet(cmuClock_LFC, cmuSelect_LFXO);
  #else
  CMU_ClockSelectSet(cmuClock_LFC, cmuSelect_LFRCO);
  #endif
  CMU_ClockEnable(cmuClock_USBLE, true);

#elif defined(_SILICON_LABS_32B_SERIES_1) && defined(_EFM32_GIANT_FAMILY)
// ------- Series 1, Giant Gecko -----------------------------------------------

  /* USB can be clocked by HFXO, USHFRCO or HFRCO+DPLL. */

  #if defined(USB_CLKSRC_HFXO)
  if (SystemHFXOClockGet() != 48000000UL) {
    DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal HFXO clock frequency");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }
  CMU_ClockSelectSet(cmuClock_USBR, cmuSelect_HFXO);

  #elif defined(USB_CLKSRC_USHFRCO)
  CMU_USHFRCOBandSet(cmuUSHFRCOFreq_48M0Hz);
  CMU_ClockSelectSet(cmuClock_USBR, cmuSelect_USHFRCO);
  /* Enable USHFRCO Clock Recovery mode. */
  CMU->USBCRCTRL |= CMU_USBCRCTRL_USBCREN;

  #elif defined(USB_CLKSRC_HFRCODPLL)
  CMU_DPLLInit_TypeDef init = CMU_DPLL_LFXO_TO_40MHZ;
  init.frequency = USB_DPLL_FREQUENCY;
  init.m = USB_DPLL_M;
  init.n = USB_DPLL_N;
  init.refClk = (CMU_DPLLClkSel_TypeDef)USB_DPLL_SRC;
  #if (USB_DPLL_SRC == USB_DPLL_SRC_LFXO)
  init.refClk = cmuDPLLClkSel_Lfxo;
  CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
  #else
  init.refClk = cmuDPLLClkSel_Hfxo;
  CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
  #endif
  CMU_ClockSelectSet(cmuClock_USBR, cmuSelect_HFRCO);
  if (!CMU_DPLLLock(&init)) {
    DEBUG_USB_API_PUTS("\nUSBD_Init(), DPLL could not lock");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  #else
  #error "Illegal USB clock selection."
  #endif

  /* LFC clock is needed to detect USB suspend when LEMIDLE is activated. */
  #if (USB_USBLEM_CLK == USB_USBLEM_CLK_LFXO)
  CMU_ClockSelectSet(cmuClock_LFC, cmuSelect_LFXO);
  #else /* (USB_USBLEM_CLK == USB_USBLEM_CLK_LFRCO) */
  CMU_ClockSelectSet(cmuClock_LFC, cmuSelect_LFRCO);
  #endif
  CMU_ClockEnable(cmuClock_USBLE, true);
#endif

  USBTIMER_Init();

  memset(dev, 0, sizeof(USBD_Device_TypeDef) );

  dev->setup                = dev->setupPkt;
  dev->deviceDescriptor     = p->deviceDescriptor;
  dev->configDescriptor     = (USB_ConfigurationDescriptor_TypeDef*)
                              p->configDescriptor;
  dev->stringDescriptors    = p->stringDescriptors;
  dev->numberOfStrings      = p->numberOfStrings;
  dev->state                = USBD_STATE_LASTMARKER;
  dev->savedState           = USBD_STATE_NONE;
  dev->lastState            = USBD_STATE_NONE;
  dev->callbacks            = p->callbacks;
  dev->remoteWakeupEnabled  = false;

  /* Initialize EP0 */

  ep                 = &dev->ep[0];
  ep->in             = false;
  ep->buf            = NULL;
  ep->num            = 0;
  ep->mask           = 1;
  ep->addr           = 0;
  ep->type           = USB_EPTYPE_CTRL;
  ep->txFifoNum      = 0;
  ep->packetSize     = p->deviceDescriptor->bMaxPacketSize0;

#if defined(_USB_DOEP0CTL_MPS_64B)
  if ( ep->packetSize == 32 ) {
    dev->ep0MpsCode = _USB_DOEP0CTL_MPS_32B;
  } else if ( ep->packetSize == 64 ) {
    dev->ep0MpsCode = _USB_DOEP0CTL_MPS_64B;
#else
  if ( ep->packetSize == 32 ) {
    dev->ep0MpsCode = 1;
  } else if ( ep->packetSize == 64 ) {
    dev->ep0MpsCode = 0;
#endif
  } else {
    return USB_STATUS_ILLEGAL;
  }

  ep->remaining      = 0;
  ep->xferred        = 0;
  ep->state          = D_EP_IDLE;
  ep->xferCompleteCb = NULL;
  ep->fifoSize       = ep->packetSize / 4;

  totalTxFifoSize = ep->fifoSize * p->bufferingMultiplier[0];
  totalRxFifoSize = (ep->fifoSize + 1) * p->bufferingMultiplier[0];

#if defined(DEBUG_USB_API)
  /* Do a sanity check on the configuration descriptor */
  {
    int i;

    /* Check if bLength's adds up exactly to wTotalLength */
    i = 0;
    conf = (uint8_t*)dev->configDescriptor;
    confEnd = conf + dev->configDescriptor->wTotalLength;

    while ( conf < confEnd ) {
      if ( *conf == 0 ) {
        break;
      }

      i += *conf;
      conf += *conf;
    }

    if ((conf != confEnd)
        || (i != dev->configDescriptor->wTotalLength)) {
      DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal configuration descriptor");
      EFM_ASSERT(false);
      return USB_STATUS_ILLEGAL;
    }
  }
#endif /* defined( DEBUG_USB_API ) */

  /* Parse configuration decriptor */
  numEps = 0;
  numInEps  = 0;
  numOutEps = 0;
  conf = (uint8_t*)dev->configDescriptor;
  confEnd = conf + dev->configDescriptor->wTotalLength;

  txFifoNum = 1;
  while ( conf < confEnd ) {
    if ( *conf == 0 ) {
      DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal configuration descriptor");
      EFM_ASSERT(false);
      return USB_STATUS_ILLEGAL;
    }

    if ( *(conf + 1) == USB_ENDPOINT_DESCRIPTOR ) {
      numEps++;
      epd = (USB_EndpointDescriptor_TypeDef*)conf;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
      ep                 = &dev->ep[numEps];
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

      ep->in             = (epd->bEndpointAddress & USB_SETUP_DIR_MASK) != 0;
      ep->buf            = NULL;
      ep->addr           = epd->bEndpointAddress;
      ep->num            = ep->addr & USB_EPNUM_MASK;
      ep->mask           = 1 << ep->num;
      ep->type           = epd->bmAttributes & CONFIG_DESC_BM_TRANSFERTYPE;
      ep->packetSize     = epd->wMaxPacketSize;
      ep->remaining      = 0;
      ep->xferred        = 0;
      ep->state          = D_EP_IDLE;
      ep->xferCompleteCb = NULL;

      if ( p->bufferingMultiplier[numEps] == 0 ) {
        DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal EP fifo buffer configuration");
        EFM_ASSERT(false);
        return USB_STATUS_ILLEGAL;
      }

      if ( ep->in ) {
        numInEps++;
        ep->txFifoNum = txFifoNum++;
        ep->fifoSize = ( (ep->packetSize + 3) / 4)
                       * p->bufferingMultiplier[numEps];
        dev->inEpAddr2EpIndex[ep->num] = numEps;
        totalTxFifoSize += ep->fifoSize;
        if ( ep->num > MAX_NUM_IN_EPS ) {
          DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal IN EP address");
          EFM_ASSERT(false);
          return USB_STATUS_ILLEGAL;
        }
      } else {
        numOutEps++;
        ep->fifoSize = ( ( (ep->packetSize + 3) / 4) + 1)
                       * p->bufferingMultiplier[numEps];
        dev->outEpAddr2EpIndex[ep->num] = numEps;
        totalRxFifoSize += ep->fifoSize;
        if ( ep->num > MAX_NUM_OUT_EPS ) {
          DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal OUT EP address");
          EFM_ASSERT(false);
          return USB_STATUS_ILLEGAL;
        }
      }
    } else if ( *(conf + 1) == USB_INTERFACE_DESCRIPTOR ) {
      id = (USB_InterfaceDescriptor_TypeDef*)conf;

      if ( id->bAlternateSetting == 0 ) {   // Only check default interfaces
        if ( dev->numberOfInterfaces != id->bInterfaceNumber ) {
          DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal interface number");
          EFM_ASSERT(false);
          return USB_STATUS_ILLEGAL;
        }
        dev->numberOfInterfaces++;
      }
    }

    conf += *conf;
  }

  /* Rx-FIFO size: SETUP packets : 4*n + 6    n=#CTRL EP's
   *               GOTNAK        : 1
   *               Status info   : 2*n        n=#OUT EP's (EP0 included) in HW
   */
  totalRxFifoSize += 10 + 1 + (2 * (MAX_NUM_OUT_EPS + 1) );

  if ( dev->configDescriptor->bNumInterfaces != dev->numberOfInterfaces ) {
    DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal interface count");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ( numEps != NUM_EP_USED ) {
    DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal EP count");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ( numInEps > MAX_NUM_IN_EPS ) {
    DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal IN EP count");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ( numOutEps > MAX_NUM_OUT_EPS ) {
    DEBUG_USB_API_PUTS("\nUSBD_Init(), Illegal OUT EP count");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  CORE_ENTER_ATOMIC();

  /* Enable USB clocks. */
#if defined(_SILICON_LABS_32B_SERIES_0)
  CMU->HFCORECLKEN0 |= CMU_HFCORECLKEN0_USB | CMU_HFCORECLKEN0_USBC;
#else
  CMU->HFBUSCLKEN0  |= CMU_HFBUSCLKEN0_USB;
  CMU->USBCTRL      |= CMU_USBCTRL_USBCLKEN;
#endif

  /* Perform final USB clock selection. */
#if defined(_SILICON_LABS_32B_SERIES_0) && !defined(_EFM32_HAPPY_FAMILY)
  CMU_ClockSelectSet(cmuClock_USBC, cmuSelect_HFCLK);
#elif defined(_EFM32_HAPPY_FAMILY)
  CMU_USHFRCOBandSet(cmuUSHFRCOBand_48MHz);
  CMU_ClockSelectSet(cmuClock_USBC, cmuSelect_USHFRCO);
  /* Enable USHFRCO Clock Recovery mode. */
  CMU->USBCRCTRL |= CMU_USBCRCTRL_EN;
#endif

  /* Turn on Low Energy Mode (LEM) features. */
#if defined(_USB_LEMCTRL_TIMEBASE_MASK)
  /* Number of LFC clock cycles (32768Hz) per 3 ms. */
  USB->LEMCTRL = (USB->LEMCTRL & ~_USB_LEMCTRL_TIMEBASE_MASK)
                 | USB_LEMCTRL_TIMEBASE_DEFAULT;
#endif
#if defined(USB_CTRL_LEMPHYCTRL)
  USB->CTRL = USB_CTRL_LEMOSCCTRL_GATE | USB_CTRL_LEMIDLEEN
              | USB_CTRL_LEMPHYCTRL;
#endif

#if defined(_EFM32_HAPPY_FAMILY)
  SYSTEM_ChipRevisionGet(&chipRev);
  if (!((chipRev.major == 1) && (chipRev.minor == 0))) {
    /* First Happy Gecko chip revision did not have all LEM features enabled. */
    USB->CTRL |= USB_CTRL_LEMNAKEN | USB_CTRL_LEMADDRMEN;
  }
#endif

  USBHAL_DisableGlobalInt();

  if ( USBDHAL_CoreInit(totalRxFifoSize, totalTxFifoSize) == USB_STATUS_OK ) {
    USBDHAL_EnableUsbResetAndSuspendInt();
    USBHAL_EnableGlobalInt();
    NVIC_ClearPendingIRQ(USB_IRQn);
    NVIC_EnableIRQ(USB_IRQn);
  } else {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_Init(), FIFO setup error");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

#if (USB_PWRSAVE_MODE & USB_PWRSAVE_MODE_ONVBUSOFF)
  if ( USBHAL_VbusIsOn() ) {
    USBD_SetUsbState(USBD_STATE_POWERED);
  } else
#endif
  {
    USBD_SetUsbState(USBD_STATE_NONE);
  }

  CORE_EXIT_ATOMIC();
  return USB_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   Start a read (OUT) transfer on an endpoint.
 *
 * @note
 *   The transfer buffer length must be a multiple of 4 bytes in length and
 *   WORD (4 byte) aligned. When allocating the buffer, round buffer length up.
 *   If it is possible that the host will send more data than your device
 *   expects, round buffer size up to the next multiple of maxpacket size.
 *
 * @param[in] epAddr
 *   Endpoint address.
 *
 * @param[in] data
 *   Pointer to transfer data buffer.
 *
 * @param[in] byteCount
 *   Transfer length.
 *
 * @param[in] callback
 *   Function to be called on transfer completion. Supply NULL if no callback
 *   is needed. See @ref USB_XferCompleteCb_TypeDef.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code enumerated
 *   in @ref USB_Status_TypeDef.
 ******************************************************************************/
int USBD_Read(int epAddr, void *data, int byteCount,
              USB_XferCompleteCb_TypeDef callback)
{
  CORE_DECLARE_IRQ_STATE;
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr(epAddr);

  if ( ep == NULL ) {
    DEBUG_USB_API_PUTS("\nUSBD_Read(), Illegal endpoint");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ((byteCount > MAX_XFER_LEN)
      || ( (byteCount / ep->packetSize) > MAX_PACKETS_PR_XFER)) {
    DEBUG_USB_API_PUTS("\nUSBD_Read(), Illegal transfer size");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ( (uint32_t)data & 3 ) {
    DEBUG_USB_API_PUTS("\nUSBD_Read(), Misaligned data buffer");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  CORE_ENTER_ATOMIC();
  if ( USBDHAL_EpIsStalled(ep) ) {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_Read(), Endpoint is halted");
    return USB_STATUS_EP_STALLED;
  }

  if ( ep->state != D_EP_IDLE ) {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_Read(), Endpoint is busy");
    return USB_STATUS_EP_BUSY;
  }

  if ( (ep->num > 0) && (USBD_GetUsbState() != USBD_STATE_CONFIGURED) ) {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_Read(), Device not configured");
    return USB_STATUS_DEVICE_UNCONFIGURED;
  }

  ep->buf       = (uint8_t*)data;
  ep->remaining = byteCount;
  ep->xferred   = 0;

  if ( ep->num == 0 ) {
    ep->in = false;
  } else if ( ep->in != false ) {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_Read(), Illegal EP direction");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  ep->state          = D_EP_RECEIVING;
  ep->xferCompleteCb = callback;

  USBD_ArmEp(ep);
  CORE_EXIT_ATOMIC();
  return USB_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   Perform a remote wakeup signalling sequence.
 *
 * @note
 *   It is the responsibility of the application to ensure that remote wakeup
 *   is not attempted before the device has been suspended for at least 5
 *   miliseconds. This function should not be called from within an interrupt
 *   handler.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code enumerated
 *   in @ref USB_Status_TypeDef.
 ******************************************************************************/
int USBD_RemoteWakeup(void)
{
  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_ATOMIC();

  if ((dev->state != USBD_STATE_SUSPENDED)
      || (dev->remoteWakeupEnabled == false)) {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_RemoteWakeup(), Illegal remote wakeup");
    return USB_STATUS_ILLEGAL;
  }

  USBDINT_RemoteWakeup();
  CORE_EXIT_ATOMIC();
  return USB_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   Check if it is ok to enter energy mode EM2.
 *
 * @note
 *   Before entering EM2 both the USB hardware and the USB stack must be in a
 *   certain state, this function checks if all conditions for entering EM2
 *   is met.
 *   Refer to the @ref usb_device_powersave section for more information.
 *
 * @return
 *   True if ok to enter EM2, false otherwise.
 ******************************************************************************/
bool USBD_SafeToEnterEM2(void)
{
#if (USB_PWRSAVE_MODE)
  return USBD_poweredDown ? true : false;
#else
  return false;
#endif
}

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

void USBD_SetUsbState(USBD_State_TypeDef newState)
{
  USBD_State_TypeDef currentState;

  currentState = dev->state;
  if ( newState == USBD_STATE_SUSPENDED ) {
    dev->savedState = currentState;
  }

  dev->lastState = dev->state;
  dev->state = newState;

  if ((dev->callbacks->usbStateChange)
      && (currentState != newState)) {
    dev->callbacks->usbStateChange(currentState, newState);
  }
}

/** @endcond */

/***************************************************************************//**
 * @brief
 *   Set an endpoint in the stalled (halted) state.
 *
 * @param[in] epAddr
 *   The address of the endpoint to stall.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code enumerated
 *   in @ref USB_Status_TypeDef.
 ******************************************************************************/
int USBD_StallEp(int epAddr)
{
  USB_Status_TypeDef retVal;
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr(epAddr);

  if ( ep == NULL ) {
    DEBUG_USB_API_PUTS("\nUSBD_StallEp(), Illegal request");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ( ep->num == 0 ) {
    DEBUG_USB_API_PUTS("\nUSBD_StallEp(), Illegal endpoint");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  CORE_ATOMIC_SECTION(
    retVal = USBDHAL_StallEp(ep);
    )

  if ( retVal != USB_STATUS_OK ) {
    retVal = USB_STATUS_ILLEGAL;
  }

  return retVal;
}

/***************************************************************************//**
 * @brief
 *   Stop USB device stack operation.
 *
 * @details
 *   The data-line pullup resistor is turned off, USB interrupts are disabled,
 *   and finally the USB pins are disabled.
 ******************************************************************************/
void USBD_Stop(void)
{
  USBD_Disconnect();
  NVIC_DisableIRQ(USB_IRQn);
  USBHAL_DisableGlobalInt();
  USBHAL_DisableUsbInt();
  USBHAL_DisablePhyPins();
  USBD_SetUsbState(USBD_STATE_NONE);
  /* Turn off USB clocks. */
#if defined(_SILICON_LABS_32B_SERIES_0)
  CMU->HFCORECLKEN0 &= ~(CMU_HFCORECLKEN0_USB | CMU_HFCORECLKEN0_USBC);
#else
  USB->DATTRIM1     &= ~USB_DATTRIM1_ENDLYPULLUP;
  CMU->HFBUSCLKEN0  &= ~CMU_HFBUSCLKEN0_USB;
  CMU->USBCTRL      &= ~CMU_USBCTRL_USBCLKEN;
#endif
}

/***************************************************************************//**
 * @brief
 *   Reset stall state on a stalled (halted) endpoint.
 *
 * @param[in] epAddr
 *   The address of the endpoint to un-stall.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code enumerated
 *   in @ref USB_Status_TypeDef.
 ******************************************************************************/
int USBD_UnStallEp(int epAddr)
{
  USB_Status_TypeDef retVal;
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr(epAddr);

  if ( ep == NULL ) {
    DEBUG_USB_API_PUTS("\nUSBD_UnStallEp(), Illegal request");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ( ep->num == 0 ) {
    DEBUG_USB_API_PUTS("\nUSBD_UnStallEp(), Illegal endpoint");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  CORE_ATOMIC_SECTION(
    retVal = USBDHAL_UnStallEp(ep);
    )

  if ( retVal != USB_STATUS_OK ) {
    retVal = USB_STATUS_ILLEGAL;
  }

  return retVal;
}

/***************************************************************************//**
 * @brief
 *   Start a write (IN) transfer on an endpoint.
 *
 * @param[in] epAddr
 *   Endpoint address.
 *
 * @param[in] data
 *   Pointer to transfer data buffer. This buffer must be WORD (4 byte) aligned.
 *
 * @param[in] byteCount
 *   Transfer length.
 *
 * @param[in] callback
 *   Function to be called on transfer completion. Supply NULL if no callback
 *   is needed. See @ref USB_XferCompleteCb_TypeDef.
 *
 * @return
 *   @ref USB_STATUS_OK on success, else an appropriate error code enumerated
 *   in @ref USB_Status_TypeDef.
 ******************************************************************************/
int USBD_Write(int epAddr, void *data, int byteCount,
               USB_XferCompleteCb_TypeDef callback)
{
  CORE_DECLARE_IRQ_STATE;
  USBD_Ep_TypeDef *ep = USBD_GetEpFromAddr(epAddr);

  if ( ep == NULL ) {
    DEBUG_USB_API_PUTS("\nUSBD_Write(), Illegal endpoint");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ((byteCount > MAX_XFER_LEN)
      || ( (byteCount / ep->packetSize) > MAX_PACKETS_PR_XFER)) {
    DEBUG_USB_API_PUTS("\nUSBD_Write(), Illegal transfer size");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  if ( (uint32_t)data & 3 ) {
    DEBUG_USB_API_PUTS("\nUSBD_Write(), Misaligned data buffer");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  CORE_ENTER_ATOMIC();
  if ( USBDHAL_EpIsStalled(ep) ) {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_Write(), Endpoint is halted");
    return USB_STATUS_EP_STALLED;
  }

  if ( ep->state != D_EP_IDLE ) {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_Write(), Endpoint is busy");
    return USB_STATUS_EP_BUSY;
  }

  if ( (ep->num > 0) && (USBD_GetUsbState() != USBD_STATE_CONFIGURED) ) {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_Write(), Device not configured");
    return USB_STATUS_DEVICE_UNCONFIGURED;
  }

  ep->buf       = (uint8_t*)data;
  ep->remaining = byteCount;
  ep->xferred   = 0;

  if ( ep->num == 0 ) {
    ep->in = true;
  } else if ( ep->in != true ) {
    CORE_EXIT_ATOMIC();
    DEBUG_USB_API_PUTS("\nUSBD_Write(), Illegal EP direction");
    EFM_ASSERT(false);
    return USB_STATUS_ILLEGAL;
  }

  ep->state          = D_EP_TRANSMITTING;
  ep->xferCompleteCb = callback;

  USBD_ArmEp(ep);
  CORE_EXIT_ATOMIC();
  return USB_STATUS_OK;
}

#if defined(USB_HOST)
/***************************************************************************//**
 * @defgroup USB
 * @{
 * @brief Gecko USB HOST and DEVICE protocol stacks.
 * @}
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup USB_COMMON
 * @{
 * @brief Common parts for both HOST and DEVICE USB stacks, see @ref usb_device
 *        and @ref usb_host pages for device and host library documentation.
 * @}
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup USB_HOST
 * @{
 * @brief Gecko USB HOST protocol stack, see @ref usb_host page for detailed documentation.
 * @}
 ******************************************************************************/
#else
/***************************************************************************//**
 * @defgroup USB
 * @{
 * @brief Gecko USB DEVICE protocol stack.
 * @}
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup USB_COMMON
 * @{
 * @brief Common parts for both HOST and DEVICE USB stacks, see @ref usb_device
 *        pages for device library documentation.
 * @}
 ******************************************************************************/
#endif
/***************************************************************************//**
 * @defgroup USB_DEVICE
 * @{
 * @brief Gecko USB DEVICE protocol stack, see @ref usb_device page for detailed documentation.
 * @}
 ******************************************************************************/

/* *INDENT-OFF* */
/******** THE REST OF THE FILE IS DOCUMENTATION ONLY !**********************//**
 * @addtogroup USB
 * @{

@page usb_device USB device stack library

  The source files for the USB device stack resides in the usb directory
  and follows the naming convention: em_usbd<em>nnn</em>.c/h.

  @li @ref usb_device_intro
  @li @ref usb_device_api
  @li @ref usb_device_conf
  @li @ref usb_device_powersave
  @li @ref usb_device_example1

@n @section usb_device_intro Introduction

  The USB device protocol stack provides an API which makes it possible to
  create USB devices with a minimum of effort. The device stack supports control,
  bulk, interrupt and isochronous transfers.

  The stack is highly configurable to suit various needs, it does also contain
  useful debugging features together with several demonstration projects to
  get you started fast.

  We recommend that you read through this documentation, then proceed to build
  and test a few example projects before you start designing your own device.

@n @section usb_device_api The device stack API

  This section contains brief descriptions of the functions in the API. You will
  find detailed information on input and output parameters and return values by
  clicking on the hyperlinked function names. It is also a good idea to study
  the code in the USB demonstration projects.

  Your application code must include one header file: @em em_usb.h.

  All functions defined in the API can be called from within interrupt handlers.

  The USB stack use a hardware timer to keep track of time. TIMER0 is the
  default choice, refer to @ref usb_device_conf for other possibilities.
  Your application must not use the selected timer.

  <b>Pitfalls:</b>@n
    The USB peripheral will fill your receive buffers in quantities of WORD's
    (4 bytes). Transmit and receive buffers must be WORD aligned, in
    addition when allocating storage for receive buffers, round size up to
    next WORD boundary. If it is possible that the host will send more data
    than your device expects, round buffer size up to the next multiple of
    maxpacket size for the relevant endpoint to avoid data corruption.

    Transmit buffers passed to @htmlonly USBD_Write() @endhtmlonly must be
    statically allocated because @htmlonly USBD_Write() @endhtmlonly only
    initiates the transfer. When the host decide to actually perform the
    transfer, your data must be available.

  @n @ref USBD_Init() @n
    This function is called to register your device and all its properties with
    the device stack. The application must fill in a @ref USBD_Init_TypeDef
    structure prior to calling. Refer to @ref DeviceInitCallbacks for the
    optional callback functions defined within this structure. When this
    function has been called your device is ready to be enumerated by the USB
    host.

  @ref USBD_Read(), @ref USBD_Write() @n
    These functions initiate data transfers.
    @n @htmlonly USBD_Read() @endhtmlonly initiate a transfer of data @em
    from host @em to device (an @em OUT transfer in USB terminology).
    @n @htmlonly USBD_Write() @endhtmlonly initiate a transfer of data @em from
    device @em to host (an @em IN transfer).

    When the USB host actually performs the transfer, your application will be
    notified by means of a callback function which you provide (optionally).
    Refer to @ref TransferCallback for details of the callback functionality.

  @ref USBD_AbortTransfer(), @ref USBD_AbortAllTransfers() @n
    These functions terminate transfers that are initiated, but has not yet
    taken place. If a transfer is initiated with @htmlonly USBD_Read()
    or USBD_Write(), @endhtmlonly but the USB host never actually peform
    the transfers, these functions will deactivate the transfer setup to make
    the USB device endpoint hardware ready for new (and potentially) different
    transfers.

  @ref USBD_Connect(), @ref USBD_Disconnect() @n
    These functions turns the data-line (D+ or D-) pullup on or off. They can be
    used to force reenumeration. It's good practice to delay at least one second
    between @htmlonly USBD_Disconnect() and USBD_Connect() @endhtmlonly
    to allow the USB host to unload the currently active device driver.

  @ref USBD_EpIsBusy() @n
    Check if an endpoint is busy.

  @ref USBD_StallEp(), @ref USBD_UnStallEp() @n
    These functions stalls or un-stalls an endpoint. This functionality may not
    be needed by your application, but the USB device stack use them in response
    to standard setup commands SET_FEATURE and CLEAR_FEATURE. They may be useful
    when implementing some USB classes, e.g. a mass storage device use them
    extensively.

  @ref USBD_RemoteWakeup() @n
    Used in SUSPENDED state (see @ref USB_Status_TypeDef) to signal resume to
    host. It's the applications responsibility to adhere to the USB standard
    which states that a device can not signal resume before it has been
    SUSPENDED for at least 5 ms. The function will also check the configuration
    descriptor defined by the application to see if it is legal for the device
    to signal resume.

  @ref USBD_GetUsbState() @n
    Returns the device USB state (see @ref USBD_State_TypeDef). Refer to
    Figure 9-1. "Device State Diagram" in the USB revision 2.0 specification.

  @ref USBD_GetUsbStateName() @n
    Returns a text string naming a given USB device state.

  @ref USBD_SafeToEnterEM2() @n
    Check if it is ok to enter energy mode EM2. Refer to the
    @ref usb_device_powersave section for more information.

  @n @anchor TransferCallback <b>The transfer complete callback function:</b> @n
    @n USB_XferCompleteCb_TypeDef() is called when a transfer completes. It is
    called with three parameters, the status of the transfer, the number of
    bytes transferred and the number of bytes remaining. It may not always be
    needed to have a callback on transfer completion, but you should keep in
    mind that a transfer may be aborted when you least expect it. A transfer
    will be aborted if host stalls the endpoint, if host resets your device, if
    host unconfigures your device or if you unplug your device cable and the
    device is selfpowered.
    @htmlonly USB_XferCompleteCb_TypeDef() @endhtmlonly is also called if your
    application use @htmlonly USBD_AbortTransfer() or USBD_AbortAllTransfers()
    @endhtmlonly calls.
    @note This callback is called from within an interrupt handler with
          interrupts disabled.

  @n @anchor DeviceInitCallbacks <b>Optional callbacks passed to the stack via
    the @ref USBD_Init() function:</b> @n
    @n These callbacks are all optional, and it is up to the application
    programmer to decide if the application needs the functionality they
    provide.
    @note These callbacks are all called from within an interrupt handler
          with interrupts disabled.

  USBD_UsbResetCb_TypeDef() is called each time reset signalling is sensed on
    the USB wire.

  @n USBD_SofIntCb_TypeDef() is called with framenumber as a parameter on
    each SOF interrupt.

  @n USBD_DeviceStateChangeCb_TypeDef() is called whenever the device state
    change. Useful for detecting e.g. SUSPENDED state change in order to reduce
    current consumption of buspowered devices. The USB HID keyboard example
    project has a good example on how to use this callback.

  @n USBD_IsSelfPoweredCb_TypeDef() is called by the device stack when host
    queries the device with a standard setup GET_STATUS command to check if the
    device is currently selfpowered or buspowered. This feature is only
    applicable on selfpowered devices which also works when only buspower is
    available.

  @n USBD_SetupCmdCb_TypeDef() is called each time a setup command is
    received from host. Use this callback to override or extend the default
    handling of standard setup commands, and to implement class or vendor
    specific setup commands. The USB HID keyboard example project has a good
    example on how to use this callback.

  @n <b>Utility functions:</b> @n
    @n    USB_PUTCHAR() Transmit a single char on the debug serial port.
    @n @n USB_PUTS() Transmit a zero terminated string on the debug serial port.
    @n @n USB_PRINTF() Transmit "printf" formated data on the debug serial port.
    @n @n USB_GetErrorMsgString() Return an error message string for a given
          error code.
    @n @n USB_PrintErrorMsgString() Format and print a text string given an
          error code, prepends an optional user supplied leader string.
    @n @n USBTIMER_DelayMs() Active wait millisecond delay function. Can also be
          used inside interrupt handlers.
    @n @n USBTIMER_DelayUs() Active wait microsecond delay function. Can also be
          used inside interrupt handlers.
    @n @n USBTIMER_Init() Initialize the timer system. Called by @htmlonly
          USBD_Init(), @endhtmlonly but your application must call it again to
          reinitialize whenever you change the HFPERCLK frequency.
    @n @n USBTIMER_Start() Start a timer. You can configure the USB device stack
          to provide any number of timers. The timers have 1 ms resolution, your
          application is notified of timeout by means of a callback.
    @n @n USBTIMER_Stop() Stop a timer.

@n @section usb_device_conf Configuring the device stack

  Your application must provide a header file named @em usbconfig.h. This file
  must contain the following \#define's:@n @n
  @verbatim
#define USB_DEVICE       // Compile the stack for device mode.
#define NUM_EP_USED n    // Your application use 'n' endpoints in
                         // addition to endpoint 0. @endverbatim

  @n @em usbconfig.h may define the following items: @n @n
  @verbatim
#define NUM_APP_TIMERS n // Your application needs 'n' timers

#define DEBUG_USB_API    // Turn on API debug diagnostics.

// Some utility functions in the API needs printf. These
// functions have "print" in their name. This macro enables
// these functions.
#define USB_USE_PRINTF   // Enable utility print functions.

// Define a function for transmitting a single char on the serial port.
extern int RETARGET_WriteChar(char c);
#define USER_PUTCHAR  RETARGET_WriteChar

#define USB_TIMER USB_TIMERn  // Select which hardware timer the USB stack
                              // is allowed to use. Valid values are n=0,1,2...
                              // corresponding to TIMER0, TIMER1, ...
                              // If not specified, TIMER0 is used

#define USB_VBUS_SWITCH_NOT_PRESENT  // Hardware does not have a VBUS switch

#define USB_CORECLK_HFRCO   // Devices in the Happy family use USHFRCO as USB
                            // clock thus supporting crystal-less USB operation.
                            // The Cortex core can be clocked from HFRCO or
                            // HFXO. Define this macros to use HFRCO as core
                            // clock, default is HFXO.
@endverbatim

  @n You are strongly encouraged to start application development with DEBUG_USB_API
  turned on. When DEBUG_USB_API is turned on and USER_PUTCHAR is defined, useful
  debugging information will be output on the development kit serial port.
  Compiling with the DEBUG_EFM_USER flag will also enable all asserts
  in both @em emlib and in the USB stack. If asserts are enabled and
  USER_PUTCHAR defined, assert texts will be output on the serial port.

  Your application must include @em retargetserial.c if DEBUG_USB_API is defined
  and @em retargetio.c if USB_USE_PRINTF is defined.
  These files reside in the @em drivers
  directory in the software package for your development board. Refer to
  @ref usb_device_powersave for energy-saving mode configurations.

  @n Giant GG11 Gecko's are very flexible in terms of using different clock
  sources to clock the USB peripheral. The clock source selected must be 48MHz
  (2500 ppm). Select one of the following macros: @n
  @verbatim
In usbconfig.h:

#define   USB_CLKSRC_HFXO        // Use HFXO as USB clock (must be 48MHz)
#define   USB_CLKSRC_USHFRCO     // Use USHFRCO as USB clock
#define   USB_CLKSRC_HFRCODPLL   // Use HFRCO and DPLL as USB clock

// If DPLL is selected, additional settings are required. Here are two examples:

// Using DPLL with 32 kHz LFXO as reference clock:
#define USB_DPLL_FREQUENCY    48005120UL
#define USB_DPLL_M            0U
#define USB_DPLL_N            1464U
#define USB_DPLL_SRC          USB_DPLL_SRC_LFXO

// Using DPLL with 50 MHz HFXO as reference clock:
#define USB_DPLL_FREQUENCY    48000000UL
#define USB_DPLL_M            349U
#define USB_DPLL_N            335U
#define USB_DPLL_SRC          USB_DPLL_SRC_HFXO
  @endverbatim

@n @section usb_device_powersave Energy-saving modes

  The device stack provides two energy saving levels. The first level is to
  set the USB peripheral in energy saving mode, the next level is to enter
  Energy Mode 2 (EM2). These energy saving modes can be applied when the device
  is suspended by the USB host, or when when the device is not connected to a
  USB host.
  In addition to this an application can use energy modes EM1 and EM2. There
  are no restrictions on when EM1 can be entered, EM2 can only be entered
  when the USB device is suspended or detached from host.

  Energy-saving modes are selected with a \#define in @em usbconfig.h, default
  selection is to not use any energy saving modes.@n @n
  @verbatim
#define USB_PWRSAVE_MODE (USB_PWRSAVE_MODE_ONSUSPEND | USB_PWRSAVE_MODE_ENTEREM2)@endverbatim

  There are three flags available, the flags can be or'ed together as shown above.

  <b>\#define USB_PWRSAVE_MODE_ONSUSPEND</b>@n Set USB peripheral in low power
  mode on suspend.

  <b>\#define USB_PWRSAVE_MODE_ONVBUSOFF</b>@n Set USB peripheral in low power
  mode when not attached to a host. This mode assumes that the internal voltage
  regulator is used and that the VREGI pin of the chip is connected to VBUS.
  This option can not be used with bus-powered devices.

  <b>\#define USB_PWRSAVE_MODE_ENTEREM2</b>@n Enter EM2 when USB peripheral is
  in low power mode.

  When the USB peripheral is set in low power mode, it must be clocked by a 32kHz
  clock. Both LFXO and LFRCO can be used, but only LFXO guarantee USB specification
  compliance. Selection is done with a \#define in @em usbconfig.h.@n @n
  @verbatim
#define USB_USBC_32kHz_CLK   USB_USBC_32kHz_CLK_LFXO @endverbatim
  Two flags are available, <b>USB_USBC_32kHz_CLK_LFXO</b> and
  <b>USB_USBC_32kHz_CLK_LFRCO</b>. <b>USB_USBC_32kHz_CLK_LFRCO</b> is selected
  by default.

  The USB HID keyboard and Mass Storage device example projects demonstrate
  different energy-saving modes.

  <b>Example 1:</b>
  Leave all energy saving to the stack, the device enters EM2 on suspend and
  when detached from host. @n
  @verbatim
In usbconfig.h:

#define USB_PWRSAVE_MODE (USB_PWRSAVE_MODE_ONSUSPEND | USB_PWRSAVE_MODE_ONVBUSOFF | USB_PWRSAVE_MODE_ENTEREM2)
  @endverbatim

  @n <b>Example 2:</b>
  Let the stack control energy saving in the USB periheral but let your
  application control energy modes EM1 and EM2. @n
  @verbatim
In usbconfig.h:

#define USB_PWRSAVE_MODE (USB_PWRSAVE_MODE_ONSUSPEND | USB_PWRSAVE_MODE_ONVBUSOFF)

In application code:

if ( USBD_SafeToEnterEM2() ) {
  EMU_EnterEM2(true);
} else {
  EMU_EnterEM1();
} @endverbatim

  @n Giant GG11 and Happy Gecko's have further energy saving modes (LEM). These
  are activated by default, but the clocksource of the LEM module is
  configurable. @n
  @verbatim
In usbconfig.h:

#define USB_USBLEM_CLK    USB_USBLEM_CLK_LFXO
  @endverbatim
  Two clock sources are available, <b>USB_USBLEM_CLK_LFXO</b> and
  <b>USB_USBLEM_CLK_LFRCO</b>. Clock source <b>USB_USBLEM_CLK_LFRCO</b>
  is selected by default.

@n @section usb_device_example1 Vendor unique device example application

  This example represents the most simple USB device imaginable. It's purpose
  is to turn user LED's on or off under control of vendor unique setup commands.
  The device will rely on @em libusb device driver on the host, a host
  application @em EFM32-LedApp.exe is bundled with the example.

  The main() is really simple ! @n @n
  @verbatim
#include "em_usb.h"

#include "descriptors.h"

int main( void )
{
  BSP_Init(BSP_INIT_DEFAULT); // Initialize DK board register access
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  BSP_LedsSet(0);             // Turn off all LED's

  ConsoleDebugInit();         // Initialize UART for debug diagnostics

  USB_PUTS("\nEFM32 USB LED Vendor Unique Device example\n");

  USBD_Init(&initstruct);     // GO !

  //When using a debugger it is pratical to uncomment the following three
  //lines to force host to re-enumerate the device.

  //USBD_Disconnect();
  //USBTIMER_DelayMs(1000);
  //USBD_Connect();

  for (;;) {}
} @endverbatim

  @n Configure the device stack in <em>usbconfig.h</em>: @n @n
  @verbatim
#define USB_DEVICE                        // Compile stack for device mode.

// **************************************************************************
**                                                                         **
** Specify number of endpoints used (in addition to EP0).                  **
**                                                                         **
*****************************************************************************
#define NUM_EP_USED 0                     // EP0 is the only endpoint used.

// **************************************************************************
**                                                                         **
** Configure serial port debug output.                                     **
**                                                                         **
*****************************************************************************
// Prototype a function for transmitting a single char on the serial port.
extern int RETARGET_WriteChar(char c);
#define USER_PUTCHAR RETARGET_WriteChar

// Enable debug diagnostics from API functions (illegal input params etc.)
#define DEBUG_USB_API @endverbatim

  @n Define device properties and fill in USB initstruct in
  <em>descriptors.h</em>: @n @n
  @verbatim
SL_ALIGN(4)
static const USB_DeviceDescriptor_TypeDef deviceDesc SL_ATTRIBUTE_ALIGN(4)=
{
  .bLength            = USB_DEVICE_DESCSIZE,
  .bDescriptorType    = USB_DEVICE_DESCRIPTOR,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0xFF,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = USB_FS_CTRL_EP_MAXSIZE,
  .idVendor           = 0x10C4,
  .idProduct          = 0x0001,
  .bcdDevice          = 0x0000,
  .iManufacturer      = 1,
  .iProduct           = 2,
  .iSerialNumber      = 3,
  .bNumConfigurations = 1
};

SL_ALIGN(4)
static const uint8_t configDesc[] SL_ATTRIBUTE_ALIGN(4)=
{
  // *** Configuration descriptor ***
  USB_CONFIG_DESCSIZE,            // bLength
  USB_CONFIG_DESCRIPTOR,          // bDescriptorType
  USB_CONFIG_DESCSIZE +           // wTotalLength (LSB)
  USB_INTERFACE_DESCSIZE,
  (USB_CONFIG_DESCSIZE +          // wTotalLength (MSB)
  USB_INTERFACE_DESCSIZE)>>8,
  1,                              // bNumInterfaces
  1,                              // bConfigurationValue
  0,                              // iConfiguration
  CONFIG_DESC_BM_RESERVED_D7 |    // bmAttrib: Self powered
  CONFIG_DESC_BM_SELFPOWERED,
  CONFIG_DESC_MAXPOWER_mA( 100 ), // bMaxPower: 100 mA

  // *** Interface descriptor ***
  USB_INTERFACE_DESCSIZE,         // bLength
  USB_INTERFACE_DESCRIPTOR,       // bDescriptorType
  0,                              // bInterfaceNumber
  0,                              // bAlternateSetting
  NUM_EP_USED,                    // bNumEndpoints
  0xFF,                           // bInterfaceClass
  0,                              // bInterfaceSubClass
  0,                              // bInterfaceProtocol
  0,                              // iInterface
};

STATIC_CONST_STRING_DESC_LANGID( langID, 0x04, 0x09 );
STATIC_CONST_STRING_DESC( iManufacturer, 'E','n','e','r','g','y',' ',       \
                                         'M','i','c','r','o',' ','A','S' );
STATIC_CONST_STRING_DESC( iProduct     , 'V','e','n','d','o','r',' ',       \
                                         'U','n','i','q','u','e',' ',       \
                                         'L','E','D',' ',                   \
                                         'D','e','v','i','c','e' );
STATIC_CONST_STRING_DESC( iSerialNumber, '0','0','0','0','0','0',           \
                                         '0','0','1','2','3','4' );

static const void * const strings[] =
{
  &langID,
  &iManufacturer,
  &iProduct,
  &iSerialNumber
};

// Endpoint buffer sizes
// 1 = single buffer, 2 = double buffering, 3 = tripple buffering ...
static const uint8_t bufferingMultiplier[ NUM_EP_USED + 1 ] = { 1 };

static const USBD_Callbacks_TypeDef callbacks =
{
  .usbReset        = NULL,
  .usbStateChange  = NULL,
  .setupCmd        = SetupCmd,
  .isSelfPowered   = NULL,
  .sofInt          = NULL
};

static const USBD_Init_TypeDef initstruct =
{
  .deviceDescriptor    = &deviceDesc,
  .configDescriptor    = configDesc,
  .stringDescriptors   = strings,
  .numberOfStrings     = sizeof(strings)/sizeof(void*),
  .callbacks           = &callbacks,
  .bufferingMultiplier = bufferingMultiplier
};  @endverbatim

  @n Now we have to implement vendor unique USB setup commands to control the
  LED's (see callbacks variable above). Notice that the buffer variable below is
  statically allocated because @htmlonly USBD_Write() @endhtmlonly only
  initiates the transfer. When the host actually performs the transfer, the
  SetupCmd() function will have returned ! @n @n

  @verbatim
#define VND_GET_LEDS 0x10
#define VND_SET_LED  0x11

static int SetupCmd( const USB_Setup_TypeDef *setup )
{
  int retVal;
  uint16_t leds;
  static uint32_t buffer;
  uint8_t *pBuffer = (uint8_t*)&buffer;

  retVal = USB_STATUS_REQ_UNHANDLED;

  if ( setup->Type == USB_SETUP_TYPE_VENDOR ) {
    switch ( setup->bRequest ) {
      case VND_GET_LEDS:
      // ********************
        *pBuffer = BSP_LedsGet() & 0x1F;
        retVal = USBD_Write( 0, pBuffer, setup->wLength, NULL );
        break;

      case VND_SET_LED:
      // ********************
        leds = DVK_getLEDs() & 0x1F;
        if ( setup->wValue )
        {
          leds |= LED0 << setup->wIndex;
        }
        else
        {
          leds &= ~( LED0 << setup->wIndex );
        }
        BSP_LedsSet( leds );
        retVal = USB_STATUS_OK;
        break;
    }
  }

  return retVal;
}@endverbatim

 * @}**************************************************************************/

#endif /* defined( USB_DEVICE ) */
#endif /* defined( USB_PRESENT ) && ( USB_COUNT == 1 ) */
