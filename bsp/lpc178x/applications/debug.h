#ifndef DEBUGH
#define DEBUGH
//define sc_printf 
//short name for smartcontroller printf
#define sc_printf(a,b...)	rt_kprintf(a,##b)
#endif
