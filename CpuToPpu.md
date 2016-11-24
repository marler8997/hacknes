

I read that to transfer memory from CPU to PPU, you can write to address $2006 to set the destination address on the PPU, then
write to $2007 to write to the PPU memory. Note that when the CPU writes to $2007, the address in $2006 will AUTO INCREMENT.

However, sprite data cannot be transferred this way , sprites can be transferred using addresses $2003 and $2004.  However,
writing to $2004 does NOT AUTO INCREMENT the address in $2003.  So Nintendo made another way to transfer sprite data.
A single write to address $4014 will start a DMA.  The value written to $4014 is the address (divided by 256) in CPU
memory space to read from.  The DMA will copy 256 bytes of data.
