#include <spin1_api.h>

// timer2 configuration parameters
// enabled, free running, interrupt disabled,
// no pre-scale, 32 bit, one-shot mode
#define TIMER2_CONF   0x83
#define TIMER2_LOAD   0xffffffff

// log of number of tests for each size
// (using log simplifies computing average)
#define NUM_ITER_LOG  7

// interrupt priority used for tests
#define INT_PRIO      -1
#if INT_PRIO == -1
#define INT_ST        VIC_FIQST
#else
#define INT_ST        VIC_IRQST
#endif


// number of tests
uint num_iter = (1 << NUM_ITER_LOG);

// synchronisation variables
volatile uint busy = 0;

// profiling variables
uint int_cost_avg = 0;
uint int_cost_min = 0xffffffff;
uint int_cost_max = 0;


void user_event (uint null0, uint null1)
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

  // and let USER trigger know that callback is done
  busy = 0;
}

void user_trigger (uint null0, uint null1)
{
  // run the required number of iterations,
  for (uint i = 0; i < num_iter; i++)
  {
    // disable interrupts to ignore USER event until ready to profile,
    uint cpsr = spin1_int_disable ();

    // trigger USER event,
    while (spin1_trigger_user_event (NULL, NULL))
    {
      continue;
    }

    // wait for USER event to be ready,
    while ((vic[INT_ST] & (1 << SOFTWARE_INT)) == 0)
    {
      continue;
    }

    // flag test as ongoing,
    busy = 1;

    // start profiling timer,
    tc[T2_LOAD] = TIMER2_LOAD;

    // re-enable interrupts to allow USER event servicing,
    spin1_mode_restore (cpsr);

    // and wait until test finished (callback done)
    while (busy)
    {
      continue;
    }
  }

  // compute average,
  int_cost_avg = int_cost_avg >> NUM_ITER_LOG;

  // report results (in clock cycles),
  io_printf (IO_BUF, "user event cost: %4d tests\n", num_iter);

  io_printf (IO_BUF, "user event cost: avg: %4d cc [%4d cc : %4d cc]\n",
      int_cost_avg, int_cost_min, int_cost_max);

  // and exit test
  spin1_exit (0);
}

void c_main (void)
{
  io_printf (IO_BUF, "start user_event_cost_profiling\n");

  // configure timer 2 for profiling
  tc[T2_CONTROL] = TIMER2_CONF;

  // register USER event callback
  spin1_callback_on(USER_EVENT, user_event, INT_PRIO);

  // schedule background task to trigger USER events
  spin1_schedule_callback (user_trigger, 0, 0, 1);

  // go
  spin1_start(SYNC_NOWAIT);

  io_printf (IO_BUF, "finish user_event_cost_profiling\n");
}
