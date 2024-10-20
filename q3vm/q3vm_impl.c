
#include "vm.h"

intptr_t systemCalls(vm_t* vm, intptr_t* args) {
	const int id = -1 - args[0];

	switch (id)
	{
	case -1: /* PRINTF */
		return printf("%s", (const char*)VMA(1, vm));
	case -2: /* ERROR */
		return fprintf(stderr, "%s", (const char*)VMA(1, vm));
	case -3: /* MEMSET */
		if (VM_MemoryRangeValid(args[1] /*addr*/, args[3] /*len*/, vm) == 0){
			memset(VMA(1, vm), args[2], args[3]);
		}
		return args[1];
	case -4: /* MEMCPY */
		if (VM_MemoryRangeValid(args[1] /*addr*/, args[3] /*len*/, vm) == 0 &&
			VM_MemoryRangeValid(args[2] /*addr*/, args[3] /*len*/, vm) == 0){
			memcpy(VMA(1, vm), VMA(2, vm), args[3]);
		}
		return args[1];
    }
}