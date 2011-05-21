/****************************************************************************
 * arch/mips/src/pic32mx/pic32mx_timerisr.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <time.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <arch/board/board.h>

#include "clock_internal.h"
#include "up_internal.h"
#include "up_arch.h"

#include "pic32mx-config.h"
#include "pic32mx-timer.h"
#include "pic32mx-int.h"
#include "pic32mx-internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/
/* Timer Setup **************************************************************/
/* Select a timer prescale value.  Our goal is to select the timer MATCH
 * register value givent the board's periperhal clock frequency and the
 * desired system timer frequency:
 *
 *   TIMER1_MATCH = BOARD_PBCLOCK / TIMER1_PRESCALE / CLOCKS_PER_SEC
 *
 * We want the largest possible value for MATCH that is less than 65,535, the
 * maximum value for the 16-bit timer register:
 *
 *   TIMER1_PRESCALE >= BOARD_PBCLOCK / CLOCKS_PER_SEC / 65535
 *
 * Timer 1 does not have very many options for the perscaler value.  So we
 * can pick the best by brute force.  Example:
 *
 *   BOARD_PBCLOCK = 40000000
 *   CLOCKS_PER_SEC         = 100
 *   OPTIMAL_PRESCALE       = 6
 *   TIMER1_PRESCALE        = 8
 *   TIMER1_MATCH           = 50,000
 */
 
#define OPTIMAL_PRESCALE (BOARD_PBCLOCK / CLOCKS_PER_SEC / 65535)
#if OPTIMAL_PRESCALE <= 1
#  define TIMER1_CON_TCKPS    TIMER1_CON_TCKPS_1
#  define TIMER1_PRESCALE     1
#elif OPTIMAL_PRESCALE <= 8
#  define TIMER1_CON_TCKPS    TIMER1_CON_TCKPS_8
#  define TIMER1_PRESCALE     8
#elif OPTIMAL_PRESCALE <= 64
#  define TIMER1_CON_TCKPS    TIMER1_CON_TCKPS_64
#  define TIMER1_PRESCALE     64
#elif OPTIMAL_PRESCALE <= 256
#  define TIMER1_CON_TCKPS    TIMER1_CON_TCKPS_256
#  define TIMER1_PRESCALE     256
#else
#  error "This timer frequency cannot be represented"
#endif

#define TIMER1_MATCH (BOARD_PBCLOCK / TIMER1_PRESCALE / CLOCKS_PER_SEC)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Global Functions
 ****************************************************************************/

/****************************************************************************
 * Function:  up_timerisr
 *
 * Description:
 *   The timer ISR will perform a variety of services for various portions
 *   of the systems.
 *
 ****************************************************************************/

int up_timerisr(int irq, uint32_t *regs)
{
   /* Clear the pending timer interrupt */
 
   putreg32(INT_T1, PIC32MX_INT_IFS0CLR);

   /* Process timer interrupt */

   sched_process_timer();
   return 0;
}

/****************************************************************************
 * Function:  up_timerinit
 *
 * Description:
 *   This function is called during start-up to initialize
 *   the timer interrupt.
 *
 ****************************************************************************/

void up_timerinit(void)
{
  /* Configure and enable TIMER1 -- source internal (TCS=0) */

  putreg32(TIMER1_CON_TCKPS, PIC32MX_TIMER1_CON);
  putreg32(0, PIC32MX_TIMER1_CNT);
  putreg32(TIMER1_MATCH-1, PIC32MX_TIMER1_PR);
  putreg32(TIMER_CON_ON, PIC32MX_TIMER1_CONSET);

  /* Configure the timer interrupt */

  up_clrpend_irq(PIC32MX_IRQSRC_T1);
  (void)up_prioritize_irq(PIC32MX_IRQ_T1, CONFIG_PIC32MX_T1PRIO);

  /* Attach the timer interrupt vector */

  (void)irq_attach(PIC32MX_IRQ_T1, (xcpt_t)up_timerisr);

  /* And enable the timer interrupt */

  up_enable_irq(PIC32MX_IRQSRC_T1);
}