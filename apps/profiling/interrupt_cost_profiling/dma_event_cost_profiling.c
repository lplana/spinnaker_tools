#include <spin1_api.h>

// timer2 configuration parameters
// enabled, free running, interrupt disabled,
// no pre-scale, 32 bit, one-shot mode
#define TIMER2_CONF   0x83
#define TIMER2_LOAD   0xffffffff

// log of number of tests for each size
// (using log simplifies computing average)
#define NUM_ITER_LOG  7

// size in bytes of DMA transfers
// (size should not impact context switch costs)
#define DMA_SIZE      256

// number of tests
uint num_iter = (1 << NUM_ITER_LOG);

// synchronisation variables
volatile uint busy = 0;

// profiling variables
uint int_cost_avg = 0;
uint int_cost_min = 0xffffffff;
uint int_cost_max = 0;


uchar buffer[DMA_SIZE];

void dma_done (uint id, uint tag)
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

  // and let DMA trigger know that callback is done
  busy = 0;
}

void dma_trigger (uint null0, uint null1)
{
  // run the required number of iterations,
  for (uint i = 0; i < num_iter; i++)
  {
    // disable interrupts to ignore DMA done event until ready to profile,
    uint cpsr = spin1_int_disable ();

    // trigger DMA transfer,
    while (!spin1_dma_transfer (i,
				(void *) sdram,
				(void *) buffer,
				DMA_READ,
				DMA_SIZE))
    {
      continue;
    }


    // wait for DMA transfer to complete,
    while ((dma[DMA_STAT] & (1 << 10)) == 0)
    {
      continue;
    }

    // flag test as ongoing,
    busy = 1;

    // start profiling timer,
    tc[T2_LOAD] = TIMER2_LOAD;

    // re-enable interrupts to allow DMA done servicing,
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
  io_printf (IO_BUF, "dma done cost: %4d tests\n", num_iter);

  io_printf (IO_BUF, "dma done cost: avg: %4d cc [%4d cc : %4d cc]\n",
      int_cost_avg, int_cost_min, int_cost_max);

  // and exit test
  spin1_exit (0);
}

void c_main (void)
{
  io_printf (IO_BUF, "start dma_event_cost_profiling\n");

  // configure timer 2 for profiling
  tc[T2_CONTROL] = TIMER2_CONF;

  // register DMA transfer done callback
  spin1_callback_on(DMA_TRANSFER_DONE, dma_done, -1);

  // schedule background task to trigger DMA transfers
  spin1_schedule_callback (dma_trigger, 0, 0, 1);

  // go
  spin1_start(SYNC_NOWAIT);

  io_printf (IO_BUF, "finish dma_event_cost_profiling\n");
}
