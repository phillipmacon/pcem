#include "ibm.h"
#include "io.h"
#include "mem.h"
#include "cpu.h"
#include "x86.h"

#include "headland.h"

static int headland_index;
static uint8_t headland_regs[256];

void headland_write(uint16_t addr, uint8_t val, void *priv)
{
        if (addr & 1)
        {
                uint8_t old_val = headland_regs[headland_index];
                
                if (headland_index == 0xc1 && !is486) val = 0;
                headland_regs[headland_index] = val;
                pclog("Headland write %02X %02X\n",headland_index,val);
                if (headland_index == 0x82)
                {
                        if (val & 0x10)
                                mem_set_mem_state(0xf0000, 0x10000, MEM_READ_INTERNAL | MEM_WRITE_DISABLED);
                        else
                                mem_set_mem_state(0xf0000, 0x10000, MEM_READ_EXTERNAL | MEM_WRITE_INTERNAL);
                }
                else if (headland_index == 0x87)
                {
                        if ((val & 1) && !(old_val & 1))
                                softresetx86();
                }
        }
        else
                headland_index = val;
}

uint8_t headland_read(uint16_t addr, void *priv)
{
        if (addr & 1) 
        {
                if ((headland_index >= 0xc0 || headland_index == 0x20) && cpu_iscyrix)
                        return 0xff; /*Don't conflict with Cyrix config registers*/
                return headland_regs[headland_index];
        }
        return headland_index;
}

void headland_init()
{
        io_sethandler(0x0022, 0x0002, headland_read, NULL, NULL, headland_write, NULL, NULL, NULL);
}
