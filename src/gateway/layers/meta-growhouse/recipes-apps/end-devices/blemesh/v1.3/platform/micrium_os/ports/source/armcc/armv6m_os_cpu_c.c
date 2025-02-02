/*
*********************************************************************************************************
*                                              Micrium OS
*                                                 CPU
*
*                             (c) Copyright 2004; Silicon Laboratories Inc.
*                                        https://www.micrium.com
*
*********************************************************************************************************
* Licensing:
*           YOUR USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS OF A MICRIUM SOFTWARE LICENSE.
*   If you are not willing to accept the terms of an appropriate Micrium Software License, you must not
*   download or use this software for any reason.
*   Information about Micrium software licensing is available at https://www.micrium.com/licensing/
*   It is your obligation to select an appropriate license based on your intended use of the Micrium OS.
*   Unless you have executed a Micrium Commercial License, your use of the Micrium OS is limited to
*   evaluation, educational or personal non-commercial uses. The Micrium OS may not be redistributed or
*   disclosed to any third party without the written consent of Silicon Laboratories Inc.
*********************************************************************************************************
* Documentation:
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*********************************************************************************************************
* Technical Support:
*   Support is available for commercially licensed users of Micrium's software. For additional
*   information on support, you can contact info@micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            OS CPU PORT FILE
*
*
* File : armv6m_os_cpu_c.c
*********************************************************************************************************
* @note     (1) This driver targets the following:
*               Core      : ARMv6M Cortex-M
*               Toolchain : ARMCC Compiler
*********************************************************************************************************
*/

#define   OS_CPU_GLOBALS

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_cpu_c__c = "$Id: $";
#endif


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  <kernel/include/os.h>
#include  <kernel/include/os_trace.h>


/**
*********************************************************************************************************
*                                           IDLE TASK HOOK
*
* @brief    This function is called by the idle task.  This hook has been added to allow you to do
*           such things as STOP the CPU to conserve power.
*********************************************************************************************************
*/

void  OSIdleTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppIdleTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppIdleTaskHookPtr)();
    }
#endif
}


/**
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*
* @brief    This function is called by OSInit() at the beginning of OSInit().
*********************************************************************************************************
*/

void  OSInitHook (void)
{
                                                                /* 8-byte align the ISR stack.                          */
    OS_CPU_ExceptStkBase = (CPU_STK *)(OSCfg_ISRStkBasePtr + OSCfg_ISRStkSize);
    OS_CPU_ExceptStkBase = (CPU_STK *)((CPU_STK)(OS_CPU_ExceptStkBase) & 0xFFFFFFF8);

}


/**
*********************************************************************************************************
*                                           REDZONE HIT HOOK
*
* @brief    This function is called when a task's stack overflowed.
*
* @param    p_tcb        Pointer to the task control block of the offending task. NULL if ISR.
*********************************************************************************************************
*/
#if (OS_CFG_TASK_STK_REDZONE_EN == DEF_ENABLED)
void  OSRedzoneHitHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppRedzoneHitHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppRedzoneHitHookPtr)(p_tcb);
    } else {
        CPU_SW_EXCEPTION(;);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
    CPU_SW_EXCEPTION(;);
#endif
}
#endif


/**
*********************************************************************************************************
*                                         STATISTIC TASK HOOK
*
* @brief    This function is called every second by uC/OS-III's statistics task.  This allows your
*           application to add functionality to the statistics task.
*********************************************************************************************************
*/

void  OSStatTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppStatTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppStatTaskHookPtr)();
    }
#endif
}


/**
*********************************************************************************************************
*                                          TASK CREATION HOOK
*
* @brief    This function is called when a task is created.
*
* @param    p_tcb        Pointer to the task control block of the task being created.

*********************************************************************************************************
*/

void  OSTaskCreateHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskCreateHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
#endif
}


/**
*********************************************************************************************************
*                                           TASK DELETION HOOK
*
* @brief    This function is called when a task is deleted.
*
* @param    p_tcb        Pointer to the task control block of the task being deleted.
*********************************************************************************************************
*/

void  OSTaskDelHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskDelHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
#endif
}


/**
*********************************************************************************************************
*                                            TASK RETURN HOOK
*
* @brief    This function is called if a task accidentally returns.  In other words, a task should
*              either be an infinite loop or delete itself when done.
*
* @param    p_tcb        Pointer to the task control block of the task that is returning.
*********************************************************************************************************
*/

void  OSTaskReturnHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskReturnHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskReturnHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                                /* Prevent compiler warning                             */
#endif
}


/**
*********************************************************************************************************
*                                        INITIALIZE A TASK'S STACK
*
* @brief    This function is called by either OSTaskCreate() or OSTaskCreateExt() to initialize the
*              stack frame of the task being created.  This function is highly processor specific.
*
* @param    p_task       Pointer to the task entry point address.
*
* @param    p_arg        Pointer to a user supplied data area that will be passed to the task
*                        when the task first executes.
*
* @param    p_stk_base   Pointer to the base address of the stack.
*
* @param    stk_size     Size of the stack, in number of CPU_STK elements.
*
* @param    opt          Options used to alter the behavior of OS_Task_StkInit().
*                        (see OS.H for OS_TASK_OPT_xxx).
*
* @return   Always returns the location of the new top-of-stack once the processor registers have
*              been placed on the stack in the proper order.
*
* @note     (1) Interrupts are enabled when task starts executing.
*
* @note     (2) All tasks run in Thread mode, using process stack.
*
* @note     (3) The SP must be 8-byte aligned in conforming to the Procedure Call Standard for the ARM architecture
*               - (a) Section 2.1 of the  ABI for the ARM Architecture Advisory Note. SP must be 8-byte aligned
*                     on entry to AAPCS-Conforming functions states :
*                     The Procedure Call Standard for the ARM Architecture [AAPCS] requires primitive
*                     data types to be naturally aligned according to their sizes (for size = 1, 2, 4, 8 bytes).
*                     Doing otherwise creates more problems than it solves.
*                     In return for preserving the natural alignment of data, conforming code is permitted
*                     to rely on that alignment. To support aligning data allocated on the stack, the stack
*                     pointer (SP) is required to be 8-byte aligned on entry to a conforming function. In
*                     practice this requirement is met if:
*                   - (1) At each call site, the current size of the calling function's stack frame is a multiple of 8 bytes.
*                         This places an obligation on compilers and assembly language programmers.
*                   - (2) SP is a multiple of 8 when control first enters a program.
*                         This places an obligation on authors of low level OS, RTOS, and runtime library
*                         code to align SP at all points at which control first enters
*                         a body of (AAPCS-conforming) code.
*                   In turn, this requires the value of SP to be aligned to 0 modulo 8:
*                   - (3) By exception handlers, before calling AAPCS-conforming code.
*                   - (4) By OS/RTOS/run-time system code, before giving control to an application.
*               - (b) Section 2.3.1 corrective steps from the the SP must be 8-byte aligned on entry
*                     to AAPCS-conforming functions advisory note also states.
*                     " This requirement extends to operating systems and run-time code for all architecture versions
*                     prior to ARMV7 and to the A, R and M architecture profiles thereafter. 
*                   - (1) Even if the SP 8-byte aligment is not a requirement for the ARMv7M profile, the stack is aligned
*                         to 8-byte boundaries to support legacy execution enviroments.
*               - (c) Section 5.2.1.2 from the Procedure Call Standard for the ARM
*                     architecture states :  "The stack must also conform to the following
*                     constraint at a public interface:
*                   - (1) SP mod 8 = 0. The stack must be double-word aligned"
*               - (d) From the ARM Technical Support Knowledge Base. 8 Byte stack aligment.
*                     "8 byte stack alignment is a requirement of the ARM Architecture Procedure
*                     Call Standard [AAPCS]. This specifies that functions must maintain an 8 byte
*                     aligned stack address (e.g. 0x00, 0x08, 0x10, 0x18, 0x20) on all external
*                     interfaces. In practice this requirement is met if:
*                   - (1) At each external interface, the current stack pointer
*                         is a multiple of 8 bytes.
*                   - (2) Your OS maintains 8 byte stack alignment on its external interfaces
*                         e.g. on task switches"
*
* @note     (4) Exception Return Behavior(EXEC_RETURN)
*               0xFFFFFFF1 	Return to Thread mode. Exception return gets state from the Main Stack.
*                           On Return execution uses the Main Stack.
*
*               0xFFFFFFF9 	Return to Thread mode. Exception return gets state from the Main Stack.
*                           On Return execution uses the Main Stack.
*
*               0xFFFFFFFD 	Return to Thread mode. Exception return gets state from the Process Stack.
*                           On Return execution uses the Process Stack.
**********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    CPU_STK    *p_stk;


    (void)opt;                                                  /* 'opt' is not used, prevent warning                   */

    p_stk = &p_stk_base[stk_size];                              /* Load stack pointer                                   */
                                                                /* Align the stack to 8-bytes.                          */
    p_stk = (CPU_STK *)((CPU_STK)(p_stk) & 0xFFFFFFF8u);
                                                                /* Registers stacked as if auto-saved on exception      */
    *(--p_stk) = (CPU_STK)0x01000000u;                          /* xPSR                                                 */
    *(--p_stk) = (CPU_STK)p_task;                               /* Entry Point                                          */
    *(--p_stk) = (CPU_STK)OS_TaskReturn;                        /* R14 (LR)                                             */
    *(--p_stk) = (CPU_STK)0x12121212u;                          /* R12                                                  */
    *(--p_stk) = (CPU_STK)0x03030303u;                          /* R3                                                   */
    *(--p_stk) = (CPU_STK)0x02020202u;                          /* R2                                                   */
    *(--p_stk) = (CPU_STK)p_stk_limit;                          /* R1                                                   */
    *(--p_stk) = (CPU_STK)p_arg;                                /* R0 : argument                                        */
                                                                /* Remaining registers saved on process stack           */
    *(--p_stk) = (CPU_STK)0xFFFFFFFDuL;                         /* R14: EXEC_RETURN; See Note 4                         */
    *(--p_stk) = (CPU_STK)0x11111111uL;                         /* R11                                                  */
    *(--p_stk) = (CPU_STK)0x10101010uL;                         /* R10                                                  */
    *(--p_stk) = (CPU_STK)0x09090909uL;                         /* R9                                                   */
    *(--p_stk) = (CPU_STK)0x08080808uL;                         /* R8                                                   */
    *(--p_stk) = (CPU_STK)0x07070707uL;                         /* R7                                                   */
    *(--p_stk) = (CPU_STK)0x06060606uL;                         /* R6                                                   */
    *(--p_stk) = (CPU_STK)0x05050505uL;                         /* R5                                                   */
    *(--p_stk) = (CPU_STK)0x04040404uL;                         /* R4                                                   */

    return (p_stk);
}


/**
*********************************************************************************************************
*                                           TASK SWITCH HOOK
*
* @brief    This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* @note     (1) Interrupts are disabled during this call.
*
* @note     (2) It is assumed that the global pointer 'OSTCBHighRdyPtr' points to the TCB of the task
*                 that will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCurPtr' points
*                 to the task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

void  OSTaskSwHook (void)
{
#if OS_CFG_TASK_PROFILE_EN > 0u
    CPU_TS  ts;
#endif
#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    CPU_TS  int_dis_time;
#endif
#if (OS_CFG_TASK_STK_REDZONE_EN == DEF_ENABLED)
    CPU_BOOLEAN  stk_status;
#endif

#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskSwHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTaskSwHookPtr)();
    }
#endif

    OS_TRACE_TASK_SWITCHED_IN(OSTCBHighRdyPtr);

#if OS_CFG_TASK_PROFILE_EN > 0u
    ts = OS_TS_GET();
    if (OSTCBCurPtr != OSTCBHighRdyPtr) {
        OSTCBCurPtr->CyclesDelta  = ts - OSTCBCurPtr->CyclesStart;
        OSTCBCurPtr->CyclesTotal += (OS_CYCLES)OSTCBCurPtr->CyclesDelta;
    }

    OSTCBHighRdyPtr->CyclesStart = ts;
#endif

#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    int_dis_time = CPU_IntDisMeasMaxCurReset();                 /* Keep track of per-task interrupt disable time        */
    if (OSTCBCurPtr->IntDisTimeMax < int_dis_time) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
                                                                /* Keep track of per-task scheduler lock time           */
    if (OSTCBCurPtr->SchedLockTimeMax < OSSchedLockTimeMaxCur) {
        OSTCBCurPtr->SchedLockTimeMax = OSSchedLockTimeMaxCur;
    }
    OSSchedLockTimeMaxCur = (CPU_TS)0;                          /* Reset the per-task value                             */
#endif

#if (OS_CFG_TASK_STK_REDZONE_EN == DEF_ENABLED)
                                                                /* Check if stack overflowed.                           */
    stk_status = OSTaskStkRedzoneChk(DEF_NULL);
    if (stk_status != DEF_OK) {
        OSRedzoneHitHook(OSTCBCurPtr);
    }
#endif
}


/**
*********************************************************************************************************
*                                              TICK HOOK
*
* @brief    This function is called every tick.
*
* @note     (1) This function is assumed to be called from the Tick ISR.
*********************************************************************************************************
*/

void  OSTimeTickHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTimeTickHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTimeTickHookPtr)();
    }
#endif
}


/**
*********************************************************************************************************
*                                          SYS TICK HANDLER
*
* @brief    Handle the system tick (SysTick) interrupt, which is used to generate the uC/OS-III tick
*           interrupt.
*
* @note     (1) This function MUST be placed on entry 15 of the Cortex-M0 vector table.
*********************************************************************************************************
*/

void  SysTick_Handler (void)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    OSIntEnter();                                               /* Tell uC/OS-III that we are starting an ISR           */
    CPU_CRITICAL_EXIT();

    OSTimeTick();                                               /* Call uC/OS-III's OSTimeTick()                        */

    OSIntExit();                                                /* Tell uC/OS-III that we are leaving the ISR           */
}


/**
*********************************************************************************************************
*                                         INITIALIZE SYS TICK
*
* @brief    Initialize the SysTick.
*
* @param    cnts         Number of SysTick counts between two OS tick interrupts.
*
* @note     (1) This function MUST be called after OSStart() & after processor initialization.
*********************************************************************************************************
*/

void  OS_CPU_SysTickInit (CPU_INT32U  cnts)
{
    CPU_INT32U  prio;


    CPU_REG_NVIC_ST_RELOAD = cnts - 1u;

                                                                /* Set SysTick handler prio.                            */
    prio                  =  CPU_REG_NVIC_SHPRI3;
    prio                 &=  0x00FFFFFFu;
    prio                 |= (OS_CPU_CFG_SYSTICK_PRIO << 24u);

    CPU_REG_NVIC_SHPRI3   = prio;

                                                                /* Enable timer.                                        */
    CPU_REG_NVIC_ST_CTRL |= CPU_REG_NVIC_ST_CTRL_CLKSOURCE |
                            CPU_REG_NVIC_ST_CTRL_ENABLE;
                                                                /* Enable timer interrupt.                              */
    CPU_REG_NVIC_ST_CTRL |= CPU_REG_NVIC_ST_CTRL_TICKINT;
}
