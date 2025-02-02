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
*                                           BSP CAN MODULE
*
*                                            Silicon Labs
*                                              Template
*
* File : bsp_can.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <can/include/can_bus.h>
#include  <common/source/rtos/rtos_utils_priv.h>
#include  <common/include/lib_def.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

CAN_BUS_HANDLE  BSP_CAN0_BusHandle;
CAN_BUS_HANDLE  BSP_CAN1_BusHandle;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  BSP_CAN0_Open             (void);

static  void  BSP_CAN1_Open             (void);

static  void  BSP_CAN0_Close            (void);

static  void  BSP_CAN1_Close            (void);

static  void  BSP_CAN0_IntCtrl          (CAN_BUS_HANDLE  bus_handle);

static  void  BSP_CAN1_IntCtrl          (CAN_BUS_HANDLE  bus_handle);

static  void  BSP_CAN_TmrCfg            (CPU_INT32U      tmr_period);

static  void  BSP_CAN_Tmr_ISR_Handler   (void);

static  void  BSP_CAN0_ISR_Handler      (void);

static  void  BSP_CAN1_ISR_Handler      (void);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/

                                                                /* ---------------- BSP API STRUCTURE ----------------- */
const CAN_CTRLR_BSP_API  CAN0_BSP_API = {
    .Open        = BSP_CAN0_Open,
    .Close       = BSP_CAN0_Close,
    .IntCtrl     = BSP_CAN0_IntCtrl,
    .TmrCfg      = BSP_CAN_TmrCfg
};

const CAN_CTRLR_BSP_API  CAN1_BSP_API = {
    .Open        = BSP_CAN1_Open,
    .Close       = BSP_CAN1_Close,
    .IntCtrl     = BSP_CAN1_IntCtrl,
    .TmrCfg      = DEF_NULL
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL CONSTANTS
*********************************************************************************************************
*********************************************************************************************************
*/

const  CAN_CTRLR_DRV_INFO  BSP_CAN0_BSP_DrvInfo = {
    .BSP_API_Ptr        = &CAN0_BSP_API,
    .HW_Info.BaseAddr   = DEF_NULL,
    .HW_Info.InfoExtPtr = DEF_NULL,
    .HW_Info.IF_Rx      = 1u,
    .HW_Info.IF_Tx      = 0u
};

const  CAN_CTRLR_DRV_INFO  BSP_CAN1_BSP_DrvInfo = {
    .BSP_API_Ptr        = &CAN1_BSP_API,
    .HW_Info.BaseAddr   = DEF_NULL,
    .HW_Info.InfoExtPtr = DEF_NULL,
    .HW_Info.IF_Rx      = 1u,
    .HW_Info.IF_Tx      = 0u
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/**
*********************************************************************************************************
*                                             BSP_CAN0_Open()
*
* @brief    Initializes the CAN IO module.
*
* @param    p_can   Pointer to a CAN register structure.
*********************************************************************************************************
*/

static void BSP_CAN0_Open (void)
{

}


/**
*********************************************************************************************************
*                                             BSP_CAN1_Open()
*
* @brief    Initializes the CAN IO module.
*********************************************************************************************************
*/

static void BSP_CAN1_Open (void)
{

}


/**
*********************************************************************************************************
*                                             BSP_CAN0_Close()
*
* @brief    De-initializes the CAN IO module.
*********************************************************************************************************
*/

static void BSP_CAN0_Close (void)
{

}


/**
*********************************************************************************************************
*                                             BSP_CAN1_Close()
*
* @brief    De-initializes the CAN IO module.
*********************************************************************************************************
*/

static void BSP_CAN1_Close (void)
{

}


/**
*********************************************************************************************************
*                                           BSP_CAN_TmrCfg()
*
* @brief    Initialize interrupt for Timer 0 module.
*
* @param    microsecond    Timer base time in microsecond.
*********************************************************************************************************
*/

static  void  BSP_CAN_TmrCfg  (CPU_INT32U  tmr_period)
{
	PP_UNUSED_PARAM(tmr_period);
}


/**
*********************************************************************************************************
*                                           BSP_CAN0_IntCtrl()
*
* @brief    Initialize interrupt for CAN IO module.
*
* @param    bus_handle    CANBus handle structure.
*********************************************************************************************************
*/

static  void  BSP_CAN0_IntCtrl (CAN_BUS_HANDLE  bus_handle)
{
    PP_UNUSED_PARAM(bus_handle);
}


/**
*********************************************************************************************************
*                                           BSP_CAN1_IntCtrl()
*
* @brief    Initialize interrupt for CAN IO module.
*
* @param    bus_handle    CANBus handle structure.
*********************************************************************************************************
*/

static  void  BSP_CAN1_IntCtrl (CAN_BUS_HANDLE  bus_handle)
{
    PP_UNUSED_PARAM(bus_handle);
}


/**
*********************************************************************************************************
*                                         BSP_CAN0_ISR_Handler()
*
* @brief    CAN0 module ISR handler.
*********************************************************************************************************
*/

static  void  BSP_CAN0_ISR_Handler (void)
{
	
}


/**
*********************************************************************************************************
*                                         BSP_CAN1_ISR_Handler()
*
* @brief    CAN1 module ISR handler.
*********************************************************************************************************
*/

static  void  BSP_CAN1_ISR_Handler (void)
{
	
}


/**
*********************************************************************************************************
*                                         BSP_CAN_Timer_ISR_Handler()
*
* @brief    Timer0 module ISR handler.
*********************************************************************************************************
*/

static  void  BSP_CAN_Tmr_ISR_Handler(void)
{

}
