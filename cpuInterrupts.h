#pragma once

// Currently the ppu sets the NMI flag by setting it
// in this extern variable so the CPU can see it.
#define IRQ_FLAG             0x01 // (Maskable Interrupt)
#define RESET_INTERRUPT_FLAG 0x02 // (Reset Interrupt)
#define NMI_FLAG             0x04 // (Non-Maskable Interrupt)
extern ubyte interruptFlags;

