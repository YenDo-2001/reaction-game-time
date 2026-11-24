/*
 * FreeRTOS Kernel V11.2.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configCPU_CLOCK_HZ                            ((unsigned long)32000000)
#define configTICK_RATE_HZ                            1000
#define configUSE_PREEMPTION                          1
#define configUSE_TIME_SLICING                        0
#define configMAX_PRIORITIES                          5UL
#define configMINIMAL_STACK_SIZE                      ((unsigned short)128)
#define configIDLE_TASK_STACK_DEPTH                   configMINIMAL_STACK_SIZE
#define configMAX_TASK_NAME_LEN                       16
#define configTICK_TYPE_WIDTH_IN_BITS                 TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                       0
#define configENABLE_MPU                              0
#define configENABLE_BACKWARD_COMPATIBILITY           0
#define configSUPPORT_STATIC_ALLOCATION               1
#define configSUPPORT_DYNAMIC_ALLOCATION              0
#define configUSE_TIMERS                              0
#define configUSE_IDLE_HOOK                           0
#define configUSE_TICK_HOOK                           0
#define configCHECK_FOR_STACK_OVERFLOW                2

#ifdef __NVIC_PRIO_BITS
#define configPRIO_BITS                               __NVIC_PRIO_BITS
#else
#define configPRIO_BITS                               2
#endif
#define configKERNEL_INTERRUPT_PRIORITY               (0x03 << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY          (0x01 << (8 - configPRIO_BITS))

#define INCLUDE_vTaskDelayUntil                       1
#define INCLUDE_vTaskDelay                            1

#define configASSERT(x) do { if (!(x)) { taskDISABLE_INTERRUPTS(); for (;;) {} } } while (0)

#ifndef __TI_COMPILER_VERSION__
#define xPortPendSVHandler                            PendSV_Handler
#define vPortSVCHandler                               SVC_Handler
#define xPortSysTickHandler                           SysTick_Handler
#endif

#endif /* FREERTOS_CONFIG_H */
