/*
*********************************************************************************************************
*                                               Micrium OS
*                                                   CPU
*
*                               (c) Copyright 2004; Silicon Laboratories Inc.
*                                           https://www.micrium.com
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
*                                       CPU CONFIGURATION DEFINES
*
* File : cpu_def.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                   MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _CPU_DEF_H_
#define  _CPU_DEF_H_


/*
*********************************************************************************************************
*                                           CPU WORD CONFIGURATION
*
* Note(s) : (1) Configure CPU_CFG_ADDR_SIZE & CPU_CFG_DATA_SIZE in '[arch]_cpu_port.h' with CPU's word sizes :
*
*                   CPU_WORD_SIZE_08             8-bit word size
*                   CPU_WORD_SIZE_16            16-bit word size
*                   CPU_WORD_SIZE_32            32-bit word size
*                   CPU_WORD_SIZE_64            64-bit word size
*
*           (2) Configure CPU_CFG_ENDIAN_TYPE in '[arch]_cpu_port.h' with CPU's data-word-memory order :
*
*               (a) CPU_ENDIAN_TYPE_BIG         Big-   endian word order (CPU words' most  significant
*                                                                           octet @ lowest memory address)
*               (b) CPU_ENDIAN_TYPE_LITTLE      Little-endian word order (CPU words' least significant
*                                                                           octet @ lowest memory address)
*********************************************************************************************************
*/

                                                                /* ------------------ CPU WORD SIZE ------------------- */
#define  CPU_WORD_SIZE_08                          1u           /*  8-bit word size (in octets).                        */
#define  CPU_WORD_SIZE_16                          2u           /* 16-bit word size (in octets).                        */
#define  CPU_WORD_SIZE_32                          4u           /* 32-bit word size (in octets).                        */
#define  CPU_WORD_SIZE_64                          8u           /* 64-bit word size (in octets).                        */


                                                                /* -------------- CPU WORD-ENDIAN ORDER --------------- */
#define  CPU_ENDIAN_TYPE_NONE                      0u
#define  CPU_ENDIAN_TYPE_BIG                       1u           /* Big-   endian word order (see Note #1a).             */
#define  CPU_ENDIAN_TYPE_LITTLE                    2u           /* Little-endian word order (see Note #1b).             */


/*
*********************************************************************************************************
*                                           CPU STACK CONFIGURATION
*
* Note(s) : (1) Configure CPU_CFG_STK_GROWTH in '[arch]_cpu_port.h' with CPU's stack growth order :
*
*               (a) CPU_STK_GROWTH_LO_TO_HI     CPU stack pointer increments to the next higher  stack
*                                                   memory address after data is pushed onto the stack
*               (b) CPU_STK_GROWTH_HI_TO_LO     CPU stack pointer decrements to the next lower   stack
*                                                   memory address after data is pushed onto the stack
*********************************************************************************************************
*/

                                                                /* -------------- CPU STACK GROWTH ORDER -------------- */
#define  CPU_STK_GROWTH_NONE                       0u
#define  CPU_STK_GROWTH_LO_TO_HI                   1u           /* CPU stk incs towards higher mem addrs (see Note #1a).*/
#define  CPU_STK_GROWTH_HI_TO_LO                   2u           /* CPU stk decs towards lower  mem addrs (see Note #1b).*/


/*
*********************************************************************************************************
*                                       CRITICAL SECTION CONFIGURATION
*
* Note(s) : (1) Configure CPU_CFG_CRITICAL_METHOD with CPU's/compiler's critical section method :
*
*                                                       Enter/Exit critical sections by ...
*
*                   CPU_CRITICAL_METHOD_INT_DIS_EN      Disable/Enable interrupts
*                   CPU_CRITICAL_METHOD_STATUS_STK      Push/Pop       interrupt status onto stack
*                   CPU_CRITICAL_METHOD_STATUS_LOCAL    Save/Restore   interrupt status to local variable
*
*               (a) CPU_CRITICAL_METHOD_INT_DIS_EN  is NOT a preferred method since it does NOT support
*                   multiple levels of interrupts.  However, with some CPUs/compilers, this is the only
*                   available method.
*
*               (b) CPU_CRITICAL_METHOD_STATUS_STK    is one preferred method since it supports multiple
*                   levels of interrupts.  However, this method assumes that the compiler provides C-level
*                   &/or assembly-level functionality for the following :
*
*                       ENTER CRITICAL SECTION :
*                       (1) Push/save   interrupt status onto a local stack
*                       (2) Disable     interrupts
*
*                       EXIT  CRITICAL SECTION :
*                       (3) Pop/restore interrupt status from a local stack
*
*               (c) CPU_CRITICAL_METHOD_STATUS_LOCAL  is one preferred method since it supports multiple
*                   levels of interrupts.  However, this method assumes that the compiler provides C-level
*                   &/or assembly-level functionality for the following :
*
*                       ENTER CRITICAL SECTION :
*                       (1) Save    interrupt status into a local variable
*                       (2) Disable interrupts
*
*                       EXIT  CRITICAL SECTION :
*                       (3) Restore interrupt status from a local variable
*
*           (2) Critical section macro's most likely require inline assembly.  If the compiler does NOT
*               allow inline assembly in C source files, critical section macro's MUST call an assembly
*               subroutine defined in a 'cpu_a.asm' file located in the following software directory :
*
*                   \<CPU-Compiler Directory>\<cpu>\<compiler>\
*
*                       where
*                               <CPU-Compiler Directory>    directory path for common   CPU-compiler software
*                               <cpu>                       directory name for specific CPU
*                               <compiler>                  directory name for specific compiler
*
*           (3) (a) To save/restore interrupt status, a local variable 'cpu_sr' of type 'CPU_SR' MAY need
*                   to be declared (e.g. if 'CPU_CRITICAL_METHOD_STATUS_LOCAL' method is configured).
*
*                   (1) 'cpu_sr' local variable SHOULD be declared via the CPU_SR_ALLOC() macro which,
*                           if used, MUST be declared following ALL other local variables (see any '[arch]_cpu_port.h
*                           CRITICAL SECTION CONFIGURATION  Note #3a1').
*
*                           Example :
*
*                           void  Fnct (void)
*                           {
*                               CPU_INT08U  val_08;
*                               CPU_INT16U  val_16;
*                               CPU_INT32U  val_32;
*                               CPU_SR_ALLOC();         MUST be declared after ALL other local variables
*                                   :
*                                   :
*                           }
*
*               (b) Configure 'CPU_SR' data type with the appropriate-sized CPU data type large enough to
*                   completely store the CPU's/compiler's status word.
*********************************************************************************************************
*/

                                                                /* ----------- CPU CRITICAL SECTION METHODS ----------- */
#define  CPU_CRITICAL_METHOD_NONE                  0u
#define  CPU_CRITICAL_METHOD_INT_DIS_EN            1u           /* DIS/EN       ints                    (see Note #1a). */
#define  CPU_CRITICAL_METHOD_STATUS_STK            2u           /* Push/Pop     int status onto stk     (see Note #1b). */
#define  CPU_CRITICAL_METHOD_STATUS_LOCAL          3u           /* Save/Restore int status to local var (see Note #1c). */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of CPU def module include.                       */
