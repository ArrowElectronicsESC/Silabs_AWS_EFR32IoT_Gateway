/*
*********************************************************************************************************
*                                             EXAMPLE CODE
*********************************************************************************************************
* Licensing:
*   The licensor of this EXAMPLE CODE is Silicon Laboratories Inc.
*
*   Silicon Laboratories Inc. grants you a personal, worldwide, royalty-free, fully paid-up license to
*   use, copy, modify and distribute the EXAMPLE CODE software, or portions thereof, in any of your
*   products.
*
*   Your use of this EXAMPLE CODE is at your own risk. This EXAMPLE CODE does not come with any
*   warranties, and the licensor disclaims all implied warranties concerning performance, accuracy,
*   non-infringement, merchantability and fitness for your application.
*
*   The EXAMPLE CODE is provided "AS IS" and does not come with any support.
*
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*
*   You can contact us at: https://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                               USB BOARD SUPPORT PACKAGE (BSP) FUNCTIONS
*
*                                            SILICON LABS
*                                           EFM32GG-DK3750
*
* File : bsp_usb_dev.c
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    BSP_USB_MODULE

#include  <common/include/lib_utils.h>
#include  <common/include/rtos_path.h>
#include  <common/include/rtos_utils.h>
#include  <rtos_description.h>

#ifdef  RTOS_MODULE_USB_DEV_AVAIL

#include  <usb/include/device/usbd_core.h>
#include  <drivers/usb/include/usbd_drv.h>

#include  "../include/bsp_int.h"

#include  <em_cmu.h>
#include  <em_gpio.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* --------- EFM32 USB ROUTE REGISTER ADDRESS --------- */
#define  EFM32_REG_USB_BASE_ADDR           (CPU_INT32U)(0x400C4000u)
#define  EFM32_REG_USB_ROUTE             (*(CPU_REG32 *)(EFM32_REG_USB_BASE_ADDR + 0x18u))

                                                                /* -------- EFM32 USB I/O ROUTE REGISTER BITS --------- */
#define  EFM32_USB_ROUTE_BIT_DMPUPEN             DEF_BIT_02
#define  EFM32_USB_ROUTE_BIT_VBUSENPEN           DEF_BIT_01
#define  EFM32_USB_ROUTE_BIT_PHYPEN              DEF_BIT_00


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* ----------------- USB DEVICE FNCTS ----------------- */
static  void  BSP_USBD_EFM32GG_DK3750_Init   (USBD_DRV  *p_drv);

static  void  BSP_USBD_EFM32GG_DK3750_Conn   (void);

static  void  BSP_USBD_EFM32GG_DK3750_Disconn(void);

static  void  BSP_USBD_EFM32GG_IntHandler    (void);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           USB DEVICE CFGS
*********************************************************************************************************
*/

static  USBD_DRV  *BSP_USBD_EFM32GG_DrvPtr;                     /* USB device driver specific info.                     */


                                                                /* ---------- USB DEVICE ENDPOINTS INFO TBL ----------- */
static  USBD_DRV_EP_INFO  BSP_USBD_EFM32GG_EP_InfoTbl[] = {
    {USBD_EP_INFO_TYPE_CTRL                                                                            | USBD_EP_INFO_DIR_OUT, 0u,   64u},
    {USBD_EP_INFO_TYPE_CTRL                                                                            | USBD_EP_INFO_DIR_IN,  0u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT, 1u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_IN,  1u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT, 2u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_IN,  2u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT, 3u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_IN,  3u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT, 4u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_IN,  4u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_OUT, 5u,   64u},
    {USBD_EP_INFO_TYPE_CTRL | USBD_EP_INFO_TYPE_ISOC | USBD_EP_INFO_TYPE_BULK | USBD_EP_INFO_TYPE_INTR | USBD_EP_INFO_DIR_IN,  5u,   64u},
    {DEF_BIT_NONE                                                                                                          ,   0u,    0u}
};

                                                                /* ---------- USB DEVICE DRIVER INFORMATION ----------- */
static  USBD_DRV_INFO  BSP_USBD_EFM32GG_DrvInfoPtr = {
    .BaseAddr   = 0x40100000u,
    .MemAddr    = 0x00000000u,
    .MemSize    = 0u,
    .Spd        = USBD_DEV_SPD_FULL,
    .EP_InfoTbl = BSP_USBD_EFM32GG_EP_InfoTbl
};

                                                                /* ------------- USB DEVICE BSP STRUCTURE ------------- */
static  USBD_DRV_BSP_API   BSP_USBD_EFM32GG_DK3750_BSP_API_Ptr = {
    BSP_USBD_EFM32GG_DK3750_Init,
    BSP_USBD_EFM32GG_DK3750_Conn,
    BSP_USBD_EFM32GG_DK3750_Disconn
};

                                                                /* ----------- USB DEVICE HW INFO STRUCTURE ----------- */
const  USBD_DEV_CTRLR_HW_INFO  BSP_USBD_EFM32GG_HwInfo = {
    .DrvAPI_Ptr  = &USBD_DrvAPI_EFM32_OTG_FS,
    .DrvInfoPtr  = &BSP_USBD_EFM32GG_DrvInfoPtr,
    .BSP_API_Ptr = &BSP_USBD_EFM32GG_DK3750_BSP_API_Ptr
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      BSP_USBD_EFM32GG_DK3750_Init()
*
* Description : USB device controller board-specific initialization.
*
*                   (1) Enable    USB dev ctrl registers  and bus clock.
*                   (2) Configure USB dev ctrl interrupts.
*                   (3) Disable   USB dev transceiver Pull-up resistor in D+ line.
*                   (4) Disable   USB dev transceiver clock.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  BSP_USBD_EFM32GG_DK3750_Init (USBD_DRV  *p_drv)
{
    BSP_USBD_EFM32GG_DrvPtr = p_drv;

    if (CMU_ClockSelectGet(cmuClock_HF) != cmuSelect_HFXO) {
        CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
    }

    CMU_OscillatorEnable(cmuOsc_LFXO, true, false);

                                                                /* Enable USB clock                                     */
    CMU->HFCORECLKEN0 |= CMU_HFCORECLKEN0_USB |
                         CMU_HFCORECLKEN0_USBC;

    CMU_ClockSelectSet(cmuClock_USBC, cmuSelect_HFCLK);

    DEF_BIT_SET(EFM32_REG_USB_ROUTE, (EFM32_USB_ROUTE_BIT_VBUSENPEN |
                                      EFM32_USB_ROUTE_BIT_PHYPEN));

                                                                /* Set handler interrupt request into interrupt channel */
    CPU_INT_SRC_HANDLER_SET_KA(BSP_INT_ID_USB,
                               BSP_USBD_EFM32GG_IntHandler);

    CPU_IntSrcDis(BSP_INT_ID_USB);
}


/*
*********************************************************************************************************
*                                      BSP_USBD_EFM32GG_DK3750_Conn()
*
* Description : Connect pull-up on DP.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  BSP_USBD_EFM32GG_DK3750_Conn (void)
{
    CPU_IntSrcEn(BSP_INT_ID_USB);                               /* Enable USB interrupt                                 */
}


/*
*********************************************************************************************************
*                                    BSP_USBD_EFM32GG_DK3750_Disconn()
*
* Description : Disconnect pull-up on DP.
*
* Argument(s) : none.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  BSP_USBD_EFM32GG_DK3750_Disconn (void)
{
    CPU_IntSrcDis(BSP_INT_ID_USB);                              /* Disable USB interrupt                                */
}


/*
*********************************************************************************************************
*                                     BSP_USBD_EFM32GG_IntHandler()
*
* Description : Provide p_drv pointer to the driver ISR Handler.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  BSP_USBD_EFM32GG_IntHandler (void)
{
    USBD_DRV  *p_drv;


    p_drv = BSP_USBD_EFM32GG_DrvPtr;
    p_drv->API_Ptr->ISR_Handler(p_drv);                         /* Call the USB Device driver ISR.                      */
}

#endif                                                          /* End of RTOS_MODULE_USB_DEV_AVAIL                     */
