/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <inc/stdio.h>

int gettime(void)
{
	nmi_disable();
	// LAB 12: your code here

	nmi_enable();
	return 0;
}

void
rtc_init(void)
{
    uint8_t reg;
	nmi_disable();
	// LAB 4: your code here
    
    outb(IO_RTC_CMND, RTC_AREG);
    reg = inb(IO_RTC_DATA);
    reg &= ~15;
    reg |= 3;
    outb(IO_RTC_DATA, reg);
    
    outb(IO_RTC_CMND, RTC_BREG);
    reg = inb(IO_RTC_DATA);
    reg |= RTC_PIE;
    outb(IO_RTC_DATA, reg);
    
	nmi_enable();
}

uint8_t
rtc_check_status(void)
{
	uint8_t status = 0;
	// LAB 4: your code here
    outb(IO_RTC_CMND, RTC_CREG);
    status = inb(IO_RTC_DATA);

	return status;
}

unsigned
mc146818_read(unsigned reg)
{
	outb(IO_RTC_CMND, reg);
	return inb(IO_RTC_DATA);
}

void
mc146818_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC_CMND, reg);
	outb(IO_RTC_DATA, datum);
}

