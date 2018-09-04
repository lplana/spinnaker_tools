#include <spin1_api.h>

// timer2 configuration parameters
// enabled, free running, interrupt disabled,
// no pre-scale, 32 bit, one-shot mode
#define TIMER2_CONF   0x83
#define TIMER2_LOAD   0xffffffff

// DMA test constants
// number of different sizes to test
#define DMA_SIZES  9
// number of tests for each size
#define DMA_TSTS_LOG  7

// local and remote buffers for DMA transfers
uint * dtcm_buffer;
uint * sdram_buffer;

// number of tests for each size
uint dma_tsts = (1 << DMA_TSTS_LOG);

// data sizes for DMA accesses
uint dma_size[DMA_SIZES] = {1, 2, 4, 8, 16, 32, 64, 128, 255};

// profiling variables
uint dma_elapsed_avg[DMA_SIZES];
uint dma_elapsed_min[DMA_SIZES];
uint dma_elapsed_max[DMA_SIZES];

uint current_size;
uint current_iter;

volatile uint busy = 0;

volatile uint ttl_tsts = 0;

void dma_done (uint null0, uint null1)
{
  // get elapsed time (timer2 counts downwards!)
  uint elapsed = TIMER2_LOAD - tc[T2_COUNT];

  // flag DMAC as not busy
  busy = 0;

  // accumulate for average
  dma_elapsed_avg[current_size] += elapsed;

  // check for min and max
  if (elapsed < dma_elapsed_min[current_size])
  {
    dma_elapsed_min[current_size] = elapsed;
  }
  else if (elapsed > dma_elapsed_max[current_size])
  {
    dma_elapsed_max[current_size] = elapsed;
  }

  ++ttl_tsts;
}

void dma_start (uint null0, uint null1)
{
  // iterate for all sizes
  current_size = 0;
  while (1)
  {
    // initialise data for current size
    dma_elapsed_avg[current_size] = 0;
    dma_elapsed_min[current_size] = 0xffffffff;
    dma_elapsed_max[current_size] = 0;
    uint tsz = (dma_size[current_size] + 3) * sizeof (uint);

    // do the required number of tests for each size
    for (uint i = 0; i < dma_tsts; i++)
    {
      // start profiling timer
      tc[T2_LOAD] = TIMER2_LOAD;

      // schedule DMA transfer
      while (!spin1_dma_transfer (i,
				  (void *) sdram_buffer,
				  (void *) dtcm_buffer,
				  DMA_READ,
				  tsz)
	)
      {
	continue;
      }


      // flag DMAC as busy
      busy = 1;

      // wait for DMA to finish
      while (busy)
      {
	continue;
      }
    }

    // adjust average
    dma_elapsed_avg[current_size] =
      dma_elapsed_avg[current_size] >> DMA_TSTS_LOG;

    // report results (in nanoseconds!)
    io_printf (IO_BUF, "size: %3d avg: %4d ns [%4d ns : %4d ns]\n",
	       dma_size[current_size],
	       5 * dma_elapsed_avg[current_size],
	       5 * dma_elapsed_min[current_size],
	       5 * dma_elapsed_max[current_size]
      );

    // check if done
    if (++current_size >= DMA_SIZES)
    {
      break;
    }
  }

  // exit tests
  spin1_exit (0);
}

void c_main (void)
{
  io_printf (IO_BUF, "start sdram_dma_profiling\n");

  // allocate DMA buffers in SDRAM and DTCM
  uint buf_size = (dma_size[DMA_SIZES - 1] + 3) * sizeof (uint);
  sdram_buffer = (uint *) sark_xalloc (sv->sdram_heap,
				       buf_size, 0, ALLOC_LOCK);
  if (sdram_buffer == NULL)
  {
    io_printf (IO_BUF, "cannot allocate SDRAM memory\n");
    return;
  }

  dtcm_buffer = (uint *) sark_xalloc (sark.heap,
				       buf_size, 0, ALLOC_LOCK);

  if (dtcm_buffer == NULL)
  {
    io_printf (IO_BUF, "cannot allocate DTCM memory\n");
    return;
  }

  // configure timer 2 for profiling
  tc[T2_CONTROL] = TIMER2_CONF;

  // register DMA done callback
  spin1_callback_on(DMA_TRANSFER_DONE, dma_done, 0);

  // schedule background task that starts DMA transfers
  spin1_schedule_callback (dma_start, 0, 0, 1);

  // go
  spin1_start(SYNC_NOWAIT);

  io_printf (IO_BUF, "total number of tests: %d\n", ttl_tsts);
  io_printf (IO_BUF, "finish sdram_dma_profiling\n");
}
