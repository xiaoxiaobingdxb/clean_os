/** the best function to find memory info
 *the largest memory address is 2^64
 * @see osdev https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15.2C_EAX_.3D_0xE820
 */
static inline void detec_memory_15_e820()
{
}

/**
 * return 2 block
 * first is memory info 0~15MB
 * second is memory info 16MB~4GB
 * @see https://wiki.osdev.org/Detecting_Memory_(x86)#BIOS_Function:_INT_0x15.2C_AX_.3D_0xE801
 */
static inline void detec_memory_15_e801()
{
}

/**
 * only find memory info 1MB~63MB
 */
static inline void detec_memory_15_88()
{
}