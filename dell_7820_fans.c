#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>


#define uint unsigned int

#define I8K_SMM_FN_STATUS	0x0025
#define I8K_SMM_POWER_STATUS	0x0069
#define I8K_SMM_SET_FAN		0x01a3
#define I8K_SMM_GET_FAN		0x00a3
#define I8K_SMM_GET_SPEED	0x02a3
#define I8K_SMM_GET_FAN_TYPE	0x03a3
#define I8K_SMM_GET_NOM_SPEED	0x04a3
#define I8K_SMM_GET_TEMP	0x10a3
#define I8K_SMM_GET_TEMP_TYPE	0x11a3
#define I8K_SMM_GET_DELL_SIG1	0xfea3
#define I8K_SMM_GET_DELL_SIG2	0xffa3

#define I8K_FAN_MULT		1

static uint i8k_fan_mult = I8K_FAN_MULT;

struct smm_regs {
	unsigned int eax;
	unsigned int ebx __attribute__ ((packed));;
	unsigned int ecx __attribute__ ((packed));;
	unsigned int edx __attribute__ ((packed));;
	unsigned int esi __attribute__ ((packed));;
	unsigned int edi __attribute__ ((packed));;
};

int init_ioperm(void)
{
    if (ioperm(0xb2, 4, 1))
        perror("ioperm:");
    if (ioperm(0x84, 4, 1))
        perror("ioperm:");
}

static int i8k_smm_func(struct smm_regs *regs)
{
	int rc;
	int eax = regs->eax;

	asm volatile("pushq %%rax\n\t"
		"movl 0(%%rax),%%edx\n\t"
		"pushq %%rdx\n\t"
		"movl 4(%%rax),%%ebx\n\t"
		"movl 8(%%rax),%%ecx\n\t"
		"movl 12(%%rax),%%edx\n\t"
		"movl 16(%%rax),%%esi\n\t"
		"movl 20(%%rax),%%edi\n\t"
		"popq %%rax\n\t"
		"out %%al,$0xb2\n\t"
		"out %%al,$0x84\n\t"
		"xchgq %%rax,(%%rsp)\n\t"
		"movl %%ebx,4(%%rax)\n\t"
		"movl %%ecx,8(%%rax)\n\t"
		"movl %%edx,12(%%rax)\n\t"
		"movl %%esi,16(%%rax)\n\t"
		"movl %%edi,20(%%rax)\n\t"
		"popq %%rdx\n\t"
		"movl %%edx,0(%%rax)\n\t"
		"pushfq\n\t"
		"popq %%rax\n\t"
		"andl $1,%%eax\n"
		: "=a"(rc)
		:    "a"(regs)
		:    "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory");

	if (rc != 0 || (regs->eax & 0xffff) == 0xffff || regs->eax == eax)
		rc = -1;

	return rc;
}

static int i8k_smm(struct smm_regs *regs)
{
	int ret;

	ret = i8k_smm_func(regs);

	return ret;
}


static int i8k_get_fan_speed(int fan)
{
	struct smm_regs regs = { .eax = I8K_SMM_GET_SPEED, };

	regs.ebx = fan & 0xff;
	return i8k_smm(&regs) ? : (regs.eax & 0xffff) * i8k_fan_mult;
}

int main()
{
   init_ioperm();
   printf("cpu0:%d\n", i8k_get_fan_speed(0));
   printf("cpu1:%d\n", i8k_get_fan_speed(1));
   printf("sys0:%d\n", i8k_get_fan_speed(2));
   printf("sys1:%d\n", i8k_get_fan_speed(3));
   printf("sys2:%d\n", i8k_get_fan_speed(4));
   printf("rear0:%d\n", i8k_get_fan_speed(5));
   printf("rear1:%d\n", i8k_get_fan_speed(6));
   return 0;
}
