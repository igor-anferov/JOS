/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <inc/stdio.h>

void
rtc_init(void)
{
    uint8_t reg;
	nmi_disable();
	// LAB 4: your code here
    
    outb(IO_RTC_CMND, RTC_AREG);
    reg = inb(IO_RTC_DATA);
    reg |= 15;
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

