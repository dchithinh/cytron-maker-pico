#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION                       1
#define configUSE_TIME_SLICING                     1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    0
#define configUSE_TICKLESS_IDLE                    0
#define configCPU_CLOCK_HZ                         ( 125000000UL )
#define configTICK_RATE_HZ                         ( 1000 )
#define configMAX_PRIORITIES                       4
#define configMINIMAL_STACK_SIZE                   128
#define configMAX_TASK_NAME_LEN                    16
#define configTICK_TYPE_WIDTH_IN_BITS              TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                    1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES      1
#define configQUEUE_REGISTRY_SIZE                  0
#define configENABLE_BACKWARD_COMPATIBILITY        0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS    0
#define configSTACK_DEPTH_TYPE                     size_t

#define configUSE_TIMERS                           1
#define configTIMER_TASK_PRIORITY                  ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                   8
#define configTIMER_TASK_STACK_DEPTH               ( configMINIMAL_STACK_SIZE )

#define configSUPPORT_DYNAMIC_ALLOCATION           1
#define configSUPPORT_STATIC_ALLOCATION            0
#define configTOTAL_HEAP_SIZE                      ( 64 * 1024 )

#define configUSE_MUTEXES                          1
#define configUSE_RECURSIVE_MUTEXES                1
#define configUSE_COUNTING_SEMAPHORES              1

#define configCHECK_FOR_STACK_OVERFLOW             2
#define configUSE_MALLOC_FAILED_HOOK               1

#define configGENERATE_RUN_TIME_STATS              0
#define configUSE_TRACE_FACILITY                   0
#define configUSE_STATS_FORMATTING_FUNCTIONS       0

#define configUSE_CO_ROUTINES                      0
#define configMAX_CO_ROUTINE_PRIORITIES            1

#define configUSE_IDLE_HOOK                        0
#define configUSE_TICK_HOOK                        0

#define configNUMBER_OF_CORES                      1
#define configUSE_CORE_AFFINITY                    0
#define configSUPPORT_PICO_TIME_INTEROP            0
#define configSUPPORT_PICO_SYNC_INTEROP            0

#define INCLUDE_vTaskPrioritySet                   1
#define INCLUDE_uxTaskPriorityGet                  1
#define INCLUDE_vTaskDelete                        1
#define INCLUDE_vTaskSuspend                       1
#define INCLUDE_xResumeFromISR                     1
#define INCLUDE_vTaskDelay                         1
#define INCLUDE_xTaskDelayUntil                    1
#define INCLUDE_xTaskGetSchedulerState             1
#define INCLUDE_xTaskGetCurrentTaskHandle          1
#define INCLUDE_uxTaskGetStackHighWaterMark        1

#define configASSERT( x )                          do { if( ( x ) == 0 ) { for( ;; ) { } } } while( 0 )

#define vPortSVCHandler                            isr_svcall
#define xPortPendSVHandler                         isr_pendsv
#define xPortSysTickHandler                        isr_systick

#endif
