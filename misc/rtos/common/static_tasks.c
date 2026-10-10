#include "FreeRTOS.h"
#include "task.h"

static StaticTask_t idle_tcb;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t timer_tcb;
static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];

void
vApplicationGetIdleTaskMemory(StaticTask_t **tcb, StackType_t **stack, uint32_t *words)
{
	*tcb = &idle_tcb;
	*stack = idle_stack;
	*words = configMINIMAL_STACK_SIZE;
}

void
vApplicationGetTimerTaskMemory(StaticTask_t **tcb, StackType_t **stack, uint32_t *words)
{
	*tcb = &timer_tcb;
	*stack = timer_stack;
	*words = configTIMER_TASK_STACK_DEPTH;
}
