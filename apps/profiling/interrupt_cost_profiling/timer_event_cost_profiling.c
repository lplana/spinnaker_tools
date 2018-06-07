#include <spin1_api.h>

// timer2 configuration parameters
// enabled, free running, interrupt disabled,
// no pre-scale, 32 bit, one-shot mode
#define TIMER2_CONF   0x83
#define TIMER2_LOAD   0xffffffff

// log of number of tests for each size
// (using log simplifies computing average)
#define NUM_ITER_LOG  7


// number of tests
uint num_iter = (1 << NUM_ITER_LOG);
uint iter = (1 << NUM_ITER_LOG);

// profiling variables
uint int_cost_avg = 0;
uint int_cost_min = 0xffffffff;
uint int_cost_max = 0;


void timer_enabler (uint, uint);


void timer_event (uint null0, uint null1)
{
  // get elapsed time (timer2 counts downwards!),
  uint elapsed = TIMER2_LOAD - tc[T2_COUNT];

  // accumulate for average,
  int_cost_avg += elapsed;

  // check for min and max,
  if (elapsed < int_cost_min)
  {
    int_cost_min = elapsed;
  }
  else if (elapsed > int_cost_max)
  {
    int_cost_max = elapsed;
  }

  // and restart TIMER enabler callback
  spin1_schedule_callback (timer_enabler, 0, 0, 1);
}

void timer_enabler (uint null0, uint null1)
{
  // run the required number of iterations,
  if (iter--)
  {
    // disable interrupts to ignore TIMER event until ready to profile,
    uint cpsr = spin1_int_disable ();

    // wait for TIMER event to be ready,
    while ((tc[T1_MASK_INT] & 1) == 0)
    {
      continue;
    }

    // start profiling timer,
    tc[T2_LOAD] = TIMER2_LOAD;

    // re-enable interrupts and return to allow TIMER event servicing
    spin1_mode_restore (cpsr);
    return;
  }

  // compute average,
  int_cost_avg = int_cost_avg >> NUM_ITER_LOG;

  // report results (in clock cycles),
  io_printf (IO_BUF, "timer event cost: %4d tests\n", num_iter);

  io_printf (IO_BUF, "timer event cost: avg: %4d cc [%4d cc : %4d cc]\n",
	     int_cost_avg, int_cost_min, int_cost_max);

  // and exit test
  spin1_exit (0);
}

void c_main (void)
{
  io_printf (IO_BUF, "start user_event_cost_profiling\n");

  // configure timer 2 for profiling,
  tc[T2_CONTROL] = TIMER2_CONF;

  // set TIMER tick period (in microseconds),
  spin1_set_timer_tick (100);

  // register USER event callback,
  spin1_callback_on(TIMER_TICK, timer_event, 1);

  // schedule background task to enable TIMER events,
  spin1_schedule_callback (timer_enabler, 0, 0, 1);

  // and go
  spin1_start(SYNC_NOWAIT);

  io_printf (IO_BUF, "finish user_event_cost_profiling\n");
}
