#include "em_device.h"
#include "em_chip.h"

#include "hal-config.h"

int main(void)
{
  /* Chip errata */
  CHIP_Init();

  /* Infinite loop */
  while (1) {
  }
}
