#include <spin1_api.h>

// ------------------------------------------------------------------------
// macros
// ------------------------------------------------------------------------
#define ROUTE_TO_CORE(p)   (1 << (p + 6))

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

// synchronisation variables
volatile uint busy = 0;

// profiling variables
uint int_cost_avg = 0;
uint int_cost_min = 0xffffffff;
uint int_cost_max = 0;


void pkt_rec (uint key, uint payload)
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

  // and let sender know that callback is done
  busy = 0;
}

void pkt_snd (uint null0, uint null1)
{
  // run the required number of iterations,
  for (uint i = 0; i < num_iter; i++)
  {
    // disable interrupts to ignore packet until ready to profile,
    uint cpsr = spin1_int_disable ();

    // send packet,
    spin1_send_mc_packet (0, 0, NO_PAYLOAD);

    // wait for packet to arrive,
    while ((cc[CC_RSR] & 1) == 0)
    {
      continue;
    }

    // flag test as ongoing
    busy = 1;

    // start profiling timer,
    tc[T2_LOAD] = TIMER2_LOAD;

    // re-enable interrupts to allow packet servicing,
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
  io_printf (IO_BUF, "interrupt cost: %4d tests\n", num_iter);

  io_printf (IO_BUF, "interrupt cost: avg: %4d cc [%4d cc : %4d cc]\n",
      int_cost_avg, int_cost_min, int_cost_max);

  // and exit test
  spin1_exit (0);
}

void c_main (void)
{
  io_printf (IO_BUF, "start interrupt_cost_profiling\n");

  // configure timer 2 for profiling
  tc[T2_CONTROL] = TIMER2_CONF;

  // setup an MC routing entry to direct all packets back to this core
  uint e = rtr_alloc (1);
  if (e)
  {
    rtr_mc_set (e, 0, 0, ROUTE_TO_CORE (spin1_get_core_id ()));
  }

  // register packet received callback
  spin1_callback_on(MC_PACKET_RECEIVED, pkt_rec,   0);
  spin1_callback_on(MCPL_PACKET_RECEIVED, pkt_rec, 0);

  // schedule background task to send packets
  spin1_schedule_callback (pkt_snd, 0, 0, 1);

  // go
  spin1_start(SYNC_NOWAIT);

  io_printf (IO_BUF, "finish interrupt_cost_profiling\n");
}
