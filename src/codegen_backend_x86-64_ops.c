#ifdef __amd64__

#include "ibm.h"
#include "codegen.h"
#include "codegen_backend.h"
#include "codegen_backend_x86-64_defs.h"
#include "codegen_backend_x86-64_ops.h"

#define RM_OP_ADD 0x00
#define RM_OP_OR  0x08
#define RM_OP_AND 0x20
#define RM_OP_SUB 0x28
#define RM_OP_XOR 0x30
#define RM_OP_CMP 0x38

#define RM_OP_SHL 0x20
#define RM_OP_SHR 0x28
#define RM_OP_SAR 0x38

static inline void codegen_addbyte(codeblock_t *block, uint8_t val)
{
        block->data[block_pos++] = val;
        if (block_pos >= BLOCK_MAX)
        {
                fatal("codegen_addbyte over! %i\n", block_pos);
//                CPU_BLOCK_END();
        }
}
static inline void codegen_addbyte2(codeblock_t *block, uint8_t vala, uint8_t valb)
{
        block->data[block_pos++] = vala;
        block->data[block_pos++] = valb;
        if (block_pos >= BLOCK_MAX)
        {
                fatal("codegen_addbyte2 over! %i\n", block_pos);
                CPU_BLOCK_END();
        }
}
static inline void codegen_addbyte3(codeblock_t *block, uint8_t vala, uint8_t valb, uint8_t valc)
{
        block->data[block_pos++] = vala;
        block->data[block_pos++] = valb;
        block->data[block_pos++] = valc;
        if (block_pos >= BLOCK_MAX)
        {
                fatal("codegen_addbyte3 over! %i\n", block_pos);
                CPU_BLOCK_END();
        }
}
static inline void codegen_addbyte4(codeblock_t *block, uint8_t vala, uint8_t valb, uint8_t valc, uint8_t vald)
{
        block->data[block_pos++] = vala;
        block->data[block_pos++] = valb;
        block->data[block_pos++] = valc;
        block->data[block_pos++] = vald;
        if (block_pos >= BLOCK_MAX)
        {
                fatal("codegen_addbyte4 over! %i\n", block_pos);
                CPU_BLOCK_END();
        }
}

static inline void codegen_addword(codeblock_t *block, uint16_t val)
{
        *(uint16_t *)&block->data[block_pos] = val;
        block_pos += 2;
        if (block_pos >= BLOCK_MAX)
        {
                fatal("codegen_addword over! %i\n", block_pos);
                CPU_BLOCK_END();
        }
}

static inline void codegen_addlong(codeblock_t *block, uint32_t val)
{
        *(uint32_t *)&block->data[block_pos] = val;
        block_pos += 4;
        if (block_pos >= BLOCK_MAX)
        {
                fatal("codegen_addlong over! %i\n", block_pos);
                CPU_BLOCK_END();
        }
}

static inline void codegen_addquad(codeblock_t *block, uint64_t val)
{
        *(uint64_t *)&block->data[block_pos] = val;
        block_pos += 8;
        if (block_pos >= BLOCK_MAX)
        {
                fatal("codegen_addquad over! %i\n", block_pos);
                CPU_BLOCK_END();
        }
}

static int is_imm8(uint32_t imm_data)
{
        if (imm_data <= 0x7f || imm_data >= 0xffffff80)
                return 1;
        return 0;
}

static inline void call(codeblock_t *block, uintptr_t func)
{
	uintptr_t diff = func - (uintptr_t)&block->data[block_pos + 5];

	if (diff >= -0x80000000 && diff < 0x7fffffff)
	{
	        codegen_addbyte(block, 0xE8); /*CALL*/
	        codegen_addlong(block, (uint32_t)diff);
	}
	else
	{
		codegen_addbyte2(block, 0x49, 0xb9); /*MOV R9, func*/
		codegen_addquad(block, func);
		codegen_addbyte3(block, 0x41, 0xff, 0xd1); /*CALL R9*/
	}
}

static inline void jmp(codeblock_t *block, uintptr_t func)
{
	uintptr_t diff = func - (uintptr_t)&block->data[block_pos + 5];

	if (diff >= -0x80000000 && diff < 0x7fffffff)
	{
	        codegen_addbyte(block, 0xe9); /*JMP*/
	        codegen_addlong(block, (uint32_t)diff);
	}
	else
	{
		codegen_addbyte2(block, 0x49, 0xb9); /*MOV R9, func*/
		codegen_addquad(block, func);
		codegen_addbyte3(block, 0x41, 0xff, 0xe1); /*JMP R9*/
	}
}

void host_x86_ADD8_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint8_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_ADD8_REG_IMM - dst_reg != src_reg\n");

        if (dst_reg == REG_EAX)
                codegen_addbyte2(block, 0x04, imm_data); /*ADD EAX, imm_data*/
        else
                codegen_addbyte3(block, 0x80, 0xc0 | RM_OP_ADD | (dst_reg & 7), imm_data); /*ADD dst_reg, imm_data*/
}
void host_x86_ADD16_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint16_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_ADD32_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte4(block, 0x66, 0x83, 0xc0 | RM_OP_ADD | (dst_reg & 7), imm_data & 0xff); /*ADD dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte2(block, 0x66, 0x05); /*AND AX, imm_data*/
                codegen_addword(block, imm_data);
        }
        else
        {
                codegen_addbyte3(block, 0x66, 0x81, 0xc0 | RM_OP_ADD | (dst_reg & 7)); /*ADD dst_reg, imm_data*/
                codegen_addword(block, imm_data);
        }
}
void host_x86_ADD32_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_ADD32_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte3(block, 0x83, 0xc0 | RM_OP_ADD | (dst_reg & 7), imm_data & 0xff); /*ADD dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte(block, 0x05); /*ADD EAX, imm_data*/
                codegen_addlong(block, imm_data);
        }
        else
        {
                codegen_addbyte2(block, 0x81, 0xc0 | RM_OP_ADD | (dst_reg & 7)); /*ADD dst_reg, imm_data*/
                codegen_addlong(block, imm_data);
        }
}
void host_x86_ADD64_REG_IMM(codeblock_t *block, int dst_reg, uint64_t imm_data)
{
        if (dst_reg & 8)
                fatal("host_x86_ADD64_REG_IMM - dst_reg & 8\n");

        if (is_imm8(imm_data))
                codegen_addbyte4(block, 0x48, 0x83, 0xc0 | RM_OP_ADD | (dst_reg & 7), imm_data & 0xff); /*ADD dst_reg, imm_data*/
        else
                fatal("ADD64_REG_IMM !is_imm8 %016llx\n", imm_data);
}
void host_x86_ADD8_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_ADD8_REG_REG - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x00, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*ADD dst_reg, src_reg_b*/
}
void host_x86_ADD16_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_ADD16_REG_REG - dst_reg != src_reg_a\n");

        codegen_addbyte3(block, 0x66, 0x01, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*ADD dst_reg, src_reg_b*/
}
void host_x86_ADD32_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_ADD32_REG_REG - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x01, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*ADD dst_reg, src_reg_b*/
}

void host_x86_ADDSD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x0f, 0x58, 0xc0 | src_reg | (dst_reg << 3));
}

void host_x86_AND8_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint8_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_AND8_REG_IMM - dst_reg != src_reg\n");

        if (dst_reg == REG_EAX)
                codegen_addbyte2(block, 0x24, imm_data); /*AND EAX, imm_data*/
        else
                codegen_addbyte3(block, 0x80, 0xc0 | RM_OP_AND | (dst_reg & 7), imm_data); /*AND dst_reg, imm_data*/
}
void host_x86_AND16_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint16_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_AND32_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte4(block, 0x66, 0x83, 0xc0 | RM_OP_AND | (dst_reg & 7), imm_data & 0xff); /*AND dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte2(block, 0x66, 0x25); /*AND AX, imm_data*/
                codegen_addword(block, imm_data);
        }
        else
        {
                codegen_addbyte3(block, 0x66, 0x81, 0xc0 | RM_OP_AND | (dst_reg & 7)); /*AND dst_reg, imm_data*/
                codegen_addword(block, imm_data);
        }
}
void host_x86_AND32_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_AND32_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte3(block, 0x83, 0xc0 | RM_OP_AND | (dst_reg & 7), imm_data & 0xff); /*AND dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte(block, 0x25); /*AND EAX, imm_data*/
                codegen_addlong(block, imm_data);
        }
        else
        {
                codegen_addbyte2(block, 0x81, 0xc0 | RM_OP_AND | (dst_reg & 7)); /*AND dst_reg, imm_data*/
                codegen_addlong(block, imm_data);
        }
}
void host_x86_AND8_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_AND8_REG_REG - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x20, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*AND dst_reg, src_reg_b*/
}
void host_x86_AND16_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_AND16_REG_REG - dst_reg != src_reg_a\n");

        codegen_addbyte3(block, 0x66, 0x21, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*AND dst_reg, src_reg_b*/
}
void host_x86_AND32_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_AND32_REG_REG - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x21, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*AND dst_reg, src_reg_b*/
}

void host_x86_CALL(codeblock_t *block, void *p)
{
        call(block, (uintptr_t)p);
}

void host_x86_CMP16_REG_IMM(codeblock_t *block, int dst_reg, uint16_t imm_data)
{
        if (is_imm8(imm_data))
                codegen_addbyte4(block, 0x66, 0x83, 0xc0 | RM_OP_CMP | dst_reg, imm_data & 0xff); /*CMP dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte2(block, 0x66, 0x3d); /*CMP AX, imm_data*/
                codegen_addword(block, imm_data);
        }
        else
        {
                codegen_addbyte3(block, 0x66, 0x81, 0xc0 | RM_OP_CMP | dst_reg); /*CMP dst_reg, imm_data*/
                codegen_addword(block, imm_data);
        }
}
void host_x86_CMP32_REG_IMM(codeblock_t *block, int dst_reg, uint32_t imm_data)
{
        if (is_imm8(imm_data))
                codegen_addbyte3(block, 0x83, 0xc0 | RM_OP_CMP | dst_reg, imm_data & 0xff); /*CMP dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte(block, 0x3d); /*CMP EAX, imm_data*/
                codegen_addlong(block, imm_data);
        }
        else
        {
                codegen_addbyte2(block, 0x81, 0xc0 | RM_OP_CMP | dst_reg); /*CMP dst_reg, imm_data*/
                codegen_addlong(block, imm_data);
        }
}
void host_x86_CMP64_REG_IMM(codeblock_t *block, int dst_reg, uint64_t imm_data)
{
        if (is_imm8(imm_data))
                codegen_addbyte4(block, 0x48, 0x83, 0xc0 | RM_OP_CMP | dst_reg, imm_data & 0xff); /*CMP dst_reg, imm_data*/
        else
                fatal("CMP64_REG_IMM not 8-bit imm\n");
}

void host_x86_CMP8_REG_REG(codeblock_t *block, int src_reg_a, int src_reg_b)
{
        codegen_addbyte2(block, 0x38, 0xc0 | src_reg_a | (src_reg_b << 3)); /*CMP src_reg_a, src_reg_b*/
}
void host_x86_CMP16_REG_REG(codeblock_t *block, int src_reg_a, int src_reg_b)
{
        codegen_addbyte3(block, 0x66, 0x39, 0xc0 | src_reg_a | (src_reg_b << 3)); /*CMP src_reg_a, src_reg_b*/
}
void host_x86_CMP32_REG_REG(codeblock_t *block, int src_reg_a, int src_reg_b)
{
        codegen_addbyte2(block, 0x39, 0xc0 | src_reg_a | (src_reg_b << 3)); /*CMP src_reg_a, src_reg_b*/
}

void host_x86_COMISD_XREG_XREG(codeblock_t *block, int src_reg_a, int src_reg_b)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x2e, 0xc0 | src_reg_b | (src_reg_a << 3));
}

void host_x86_CVTSD2SI_REG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x0f, 0x2d, 0xc0 | src_reg | (dst_reg << 3)); /*CVTSD2SI dst_reg, src_reg*/
}
void host_x86_CVTSD2SI_REG64_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x48, 0x0f, 0x2d); /*CVTSD2SI dst_reg, src_reg*/
        codegen_addbyte(block, 0xc0 | src_reg | (dst_reg << 3));
}
void host_x86_CVTSD2SS_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x0f, 0x5a, 0xc0 | src_reg | (dst_reg << 3));
}

void host_x86_CVTSI2SD_XREG_REG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x0f, 0x2a, 0xc0 | src_reg | (dst_reg << 3)); /*CVTSI2SD dst_reg, src_reg*/
}
void host_x86_CVTSI2SD_XREG_REG64(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x48, 0x0f, 0x2a); /*CVTSI2SD dst_reg, src_reg*/
        codegen_addbyte(block, 0xc0 | src_reg | (dst_reg << 3));
}

void host_x86_CVTSS2SD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf3, 0x0f, 0x5a, 0xc0 | src_reg | (dst_reg << 3));
}
void host_x86_CVTSS2SD_XREG_BASE_INDEX(codeblock_t *block, int dst_reg, int base_reg, int idx_reg)
{
        codegen_addbyte4(block, 0xf3, 0x0f, 0x5a, 0x04 | (dst_reg << 3)); /*CVTSS2SD XMMx, [base_reg + idx_reg]*/
        codegen_addbyte(block, base_reg | (idx_reg << 3));
}

void host_x86_DIVSD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x0f, 0x5e, 0xc0 | src_reg | (dst_reg << 3));
}

void host_x86_JMP(codeblock_t *block, void *p)
{
        jmp(block, (uintptr_t)p);
}

void host_x86_JNZ(codeblock_t *block, void *p)
{
        codegen_addbyte2(block, 0x0f, 0x85); /*JNZ*/
        codegen_addlong(block, (uintptr_t)p - (uintptr_t)&block->data[block_pos + 4]);
}
void host_x86_JZ(codeblock_t *block, void *p)
{
        codegen_addbyte2(block, 0x0f, 0x84); /*JZ*/
        codegen_addlong(block, (uintptr_t)p - (uintptr_t)&block->data[block_pos + 4]);
}

uint8_t *host_x86_JNZ_short(codeblock_t *block)
{
        codegen_addbyte2(block, 0x75, 0); /*JNZ*/
        return &block->data[block_pos-1];
}
uint8_t *host_x86_JS_short(codeblock_t *block)
{
        codegen_addbyte2(block, 0x78, 0); /*JS*/
        return &block->data[block_pos-1];
}
uint8_t *host_x86_JZ_short(codeblock_t *block)
{
        codegen_addbyte2(block, 0x74, 0); /*JZ*/
        return &block->data[block_pos-1];
}

uint32_t *host_x86_JNB_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x83); /*JNB*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JNBE_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x87); /*JNBE*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JNL_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x8d); /*JNL*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JNLE_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x8f); /*JNLE*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JNO_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x81); /*JNO*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JNS_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x89); /*JNS*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JNZ_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x85); /*JNZ*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JB_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x82); /*JB*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JBE_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x86); /*JBE*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JL_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x8c); /*JL*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JLE_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x8e); /*JLE*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JO_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x80); /*JO*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JS_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x88); /*JS*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}
uint32_t *host_x86_JZ_long(codeblock_t *block)
{
        codegen_addbyte2(block, 0x0f, 0x84); /*JZ*/
        codegen_addlong(block, 0);
        return (uint32_t *)&block->data[block_pos-4];
}

void host_x86_LAHF(codeblock_t *block)
{
        codegen_addbyte(block, 0x9f); /*LAHF*/
}

void host_x86_LDMXCSR(codeblock_t *block, void *p)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte4(block, 0x0f, 0xae, 0x50 | REG_EBP, offset); /*LDMXCSR offset[EBP]*/
        }
        else
        {
                fatal("host_x86_LDMXCSR - out of range %p\n", p);
        }
}

void host_x86_LEA_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t offset)
{
        if (offset)
        {
                codegen_addbyte2(block, 0x8d, 0x80 | (dst_reg << 3) | src_reg); /*LEA dst_reg, [offset+src_reg]*/
                codegen_addlong(block, offset);
        }
        else
                codegen_addbyte2(block, 0x8d, 0x00 | (dst_reg << 3) | src_reg); /*LEA dst_reg, [src_reg]*/
}
void host_x86_LEA_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if ((dst_reg & 8) || (src_reg_a & 8) || (src_reg_b & 8))
                fatal("host_x86_LEA_REG_REG - bad reg\n");

        codegen_addbyte3(block, 0x8d, 0x04 | ((dst_reg & 7) << 3),  /*LEA dst_reg, [Rsrc_reg_a + Rsrc_reg_b]*/
                         ((src_reg_b & 7) << 3) | (src_reg_a & 7));
}
void host_x86_LEA_REG_REG_SHIFT(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b, int shift)
{
        if ((dst_reg & 8) || (src_reg_a & 8) || (src_reg_b & 8))
                fatal("host_x86_LEA_REG_REG_SHIFT - bad reg\n");

        codegen_addbyte3(block, 0x8d, 0x04 | ((dst_reg & 7) << 3),  /*LEA dst_reg, [Rsrc_reg_a + Rsrc_reg_b * (1 << shift)]*/
                         (shift << 6) | ((src_reg_b & 7) << 3) | (src_reg_a & 7));
}

void host_x86_MAXSD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x0f, 0x5f, 0xc0 | src_reg | (dst_reg << 3)); /*MAXSD dst_reg, src_reg*/
}

void host_x86_MOV8_ABS_IMM(codeblock_t *block, void *p, uint32_t imm_data)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte3(block, 0xc6, 0x45, offset); /*MOVB offset[RBP], imm_data*/
                codegen_addbyte(block, imm_data);
        }
        else
        {
                if ((uintptr_t)p >> 32)
                        fatal("host_x86_MOV8_ABS_IMM - out of range %p\n", p);
                codegen_addbyte3(block, 0xc6, 0x04, 0x25); /*MOVB p, imm_data*/
                codegen_addlong(block, (uint32_t)(uintptr_t)p);
                codegen_addbyte(block, imm_data);
        }
}
void host_x86_MOV32_ABS_IMM(codeblock_t *block, void *p, uint32_t imm_data)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte3(block, 0xc7, 0x45, offset); /*MOV offset[RBP], imm_data*/
                codegen_addlong(block, imm_data);
        }
        else
        {
                if ((uintptr_t)p >> 32)
                        fatal("host_x86_MOV32_ABS_IMM - out of range %p\n", p);
                codegen_addbyte3(block, 0xc7, 0x04, 0x25); /*MOV p, imm_data*/
                codegen_addlong(block, (uint32_t)(uintptr_t)p);
                codegen_addlong(block, imm_data);
        }
}

void host_x86_MOV8_ABS_REG(codeblock_t *block, void *p, int src_reg)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (src_reg & 8)
                fatal("host_x86_MOV8_ABS_REG - bad reg\n");

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte3(block, 0x88, 0x45 | ((src_reg & 7) << 3), offset); /*MOVB offset[RBP], src_reg*/
        }
        else
        {
                if ((uintptr_t)p >> 32)
                        fatal("host_x86_MOV8_ABS_REG - out of range %p\n", p);
                codegen_addbyte(block, 0x88); /*MOVB [p], src_reg*/
                codegen_addbyte(block, 0x05 | ((src_reg & 7) << 3));
                codegen_addlong(block, (uint32_t)(uintptr_t)p);
        }
}
void host_x86_MOV16_ABS_REG(codeblock_t *block, void *p, int src_reg)
{
        int64_t offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (src_reg & 8)
                fatal("host_x86_MOV16_ABS_REG - bad reg\n");

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte4(block, 0x66, 0x89, 0x45 | ((src_reg & 7) << 3), offset); /*MOV offset[RBP], src_reg*/
        }
        else if (offset < (1ull << 32))
        {
                codegen_addbyte3(block, 0x66, 0x89, 0x85 | ((src_reg & 7) << 3)); /*MOV offset[RBP], src_reg*/
                codegen_addlong(block, offset);
        }
        else
        {
                if ((uintptr_t)p >> 32)
                        fatal("host_x86_MOV32_ABS_REG - out of range %p\n", p);
        }
}
void host_x86_MOV32_ABS_REG(codeblock_t *block, void *p, int src_reg)
{
        int64_t offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (src_reg & 8)
                fatal("host_x86_MOV32_ABS_REG - bad reg\n");

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte3(block, 0x89, 0x45 | ((src_reg & 7) << 3), offset); /*MOV offset[RBP], src_reg*/
        }
        else if (offset < (1ull << 32))
        {
                codegen_addbyte2(block, 0x89, 0x85 | ((src_reg & 7) << 3)); /*MOV offset[RBP], src_reg*/
                codegen_addlong(block, offset);
        }
        else
        {
                if ((uintptr_t)p >> 32)
                        fatal("host_x86_MOV32_ABS_REG - out of range %p\n", p);
                codegen_addbyte(block, 0x89); /*MOV [p], src_reg*/
                codegen_addbyte(block, 0x05 | ((src_reg & 7) << 3));
                codegen_addlong(block, (uint32_t)(uintptr_t)p);
        }
}
void host_x86_MOV64_ABS_REG(codeblock_t *block, void *p, int src_reg)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (src_reg & 8)
                fatal("host_x86_MOV64_ABS_REG - bad reg\n");

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte4(block, 0x48, 0x89, 0x45 | ((src_reg & 7) << 3), offset); /*MOV offset[RBP], src_reg*/
        }
        else
        {
                if ((uintptr_t)p >> 32)
                        fatal("host_x86_MOV64_ABS_REG - out of range %p\n", p);
                codegen_addbyte4(block, 0x48, 0x89, 0x04 | ((src_reg & 7) << 3), 0x25); /*MOV [p], src_reg*/
                codegen_addlong(block, (uint32_t)(uintptr_t)p);
        }
}

void host_x86_MOV8_ABS_REG_REG_SHIFT_REG(codeblock_t *block, uint32_t addr, int base_reg, int index_reg, int shift, int src_reg)
{
        if ((src_reg & 8) || (base_reg & 8) | (index_reg & 8))
                fatal("host_x86_MOV8_BASE_INDEX_REG reg & 8\n");
        if (addr < 0x80 || addr >= 0xffffff80)
        {
                codegen_addbyte4(block, 0x88, 0x44 | (src_reg << 3), base_reg | (index_reg << 3) | (shift << 6), addr & 0xff); /*MOV addr[base_reg + idx_reg << shift], src_reg*/
        }
        else
        {
                codegen_addbyte3(block, 0x88, 0x84 | (src_reg << 3), base_reg | (index_reg << 3) | (shift << 6)); /*MOV addr[base_reg + idx_reg << shift], src_reg*/
                codegen_addlong(block, addr);
        }
}

void host_x86_MOV8_BASE_INDEX_REG(codeblock_t *block, int base_reg, int index_reg, int src_reg)
{
        if ((src_reg & 8) || (base_reg & 8) | (index_reg & 8))
                fatal("host_x86_MOV8_BASE_INDEX_REG reg & 8\n");
        codegen_addbyte3(block, 0x88, 0x04 | (src_reg << 3), (index_reg << 3) | base_reg); /*MOV B[base_reg + index_reg], src_reg*/
}
void host_x86_MOV16_BASE_INDEX_REG(codeblock_t *block, int base_reg, int index_reg, int src_reg)
{
        if ((src_reg & 8) || (base_reg & 8) | (index_reg & 8))
                fatal("host_x86_MOV8_BASE_INDEX_REG reg & 8\n");
        codegen_addbyte4(block, 0x66, 0x89, 0x04 | (src_reg << 3), (index_reg << 3) | base_reg); /*MOV W[base_reg + index_reg], src_reg*/
}
void host_x86_MOV32_BASE_INDEX_REG(codeblock_t *block, int base_reg, int index_reg, int src_reg)
{
        if ((src_reg & 8) || (base_reg & 8) | (index_reg & 8))
                fatal("host_x86_MOV8_BASE_INDEX_REG reg & 8\n");
        codegen_addbyte3(block, 0x89, 0x04 | (src_reg << 3), (index_reg << 3) | base_reg); /*MOV L[base_reg + index_reg], src_reg*/
}

void host_x86_MOV8_REG_ABS(codeblock_t *block, int dst_reg, void *p)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (dst_reg & 8)
                fatal("host_x86_MOV8_REG_ABS reg & 8\n");

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte3(block, 0x8a, 0x45 | ((dst_reg & 7) << 3), offset); /*MOV dst_reg, offset[RBP]*/
        }
        else if (offset < (1ull << 32))
        {
                codegen_addbyte2(block, 0x8a, 0x85 | ((dst_reg & 7) << 3)); /*MOV dst_reg, offset[RBP]*/
                codegen_addlong(block, offset);
        }
        else
        {
                fatal("host_x86_MOV8_REG_ABS - out of range\n");
        }
}
void host_x86_MOV16_REG_ABS(codeblock_t *block, int dst_reg, void *p)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (dst_reg & 8)
                fatal("host_x86_MOV16_REG_ABS reg & 8\n");

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte4(block, 0x66, 0x8b, 0x45 | ((dst_reg & 7) << 3), offset); /*MOV dst_reg, offset[RBP]*/
        }
        else if (offset < (1ull << 32))
        {
                codegen_addbyte3(block, 0x66, 0x8b, 0x85 | ((dst_reg & 7) << 3)); /*MOV dst_reg, offset[RBP]*/
                codegen_addlong(block, offset);
        }
        else
        {
                fatal("host_x86_MOV16_REG_ABS - out of range\n");
        }
}
void host_x86_MOV32_REG_ABS(codeblock_t *block, int dst_reg, void *p)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (dst_reg & 8)
                fatal("host_x86_MOV32_REG_ABS reg & 8\n");

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte3(block, 0x8b, 0x45 | ((dst_reg & 7) << 3), offset); /*MOV dst_reg, offset[RBP]*/
        }
        else if (offset < (1ull << 32))
        {
                codegen_addbyte2(block, 0x8b, 0x85 | ((dst_reg & 7) << 3)); /*MOV dst_reg, offset[RBP]*/
                codegen_addlong(block, offset);
        }
        else
        {
                fatal("host_x86_MOV32_REG_ABS - out of range\n");
                codegen_addbyte(block, 0x8b); /*MOV [p], src_reg*/
                codegen_addbyte(block, 0x05 | ((dst_reg & 7) << 3));
                codegen_addlong(block, (uint32_t)(uintptr_t)p);
        }
}

void host_x86_MOV8_REG_ABS_REG_REG_SHIFT(codeblock_t *block, int dst_reg, uint32_t addr, int base_reg, int index_reg, int shift)
{
        if ((dst_reg & 8) || (base_reg & 8) | (index_reg & 8))
                fatal("host_x86_MOV8_REG_ABS_REG_REG_SHIFT reg & 8\n");
        if (addr < 0x80 || addr >= 0xffffff80)
        {
                codegen_addbyte4(block, 0x8a, 0x44 | (dst_reg << 3), base_reg | (index_reg << 3) | (shift << 6), addr & 0xff); /*MOV addr[base_reg + idx_reg << shift], src_reg*/
        }
        else
        {
                codegen_addbyte3(block, 0x8a, 0x84 | (dst_reg << 3), base_reg | (index_reg << 3) | (shift << 6)); /*MOV addr[base_reg + idx_reg << shift], src_reg*/
                codegen_addlong(block, addr);
        }
}

void host_x86_MOV32_REG_BASE_INDEX(codeblock_t *block, int dst_reg, int base_reg, int index_reg)
{
        if ((dst_reg & 8) || (base_reg & 8) | (index_reg & 8))
                fatal("host_x86_MOV32_REG_BASE_INDEX reg & 8\n");
        codegen_addbyte3(block, 0x8b, 0x04 | (dst_reg << 3), (index_reg << 3) | base_reg); /*MOV dst_reg, Q[base_reg + index_reg]*/
}

void host_x86_MOV64_REG_BASE_INDEX_SHIFT(codeblock_t *block, int dst_reg, int base_reg, int index_reg, int scale)
{
        if ((dst_reg & 8) || (index_reg & 8))
                fatal("host_x86_MOV64_REG_BASE_INDEX_SHIFT reg & 8\n");
        if (base_reg & 8)
                codegen_addbyte4(block, 0x49, 0x8b, 0x04 | ((dst_reg & 7) << 3), (scale << 6) | ((index_reg & 7) << 3) | (base_reg & 7)); /*MOV dst_reg, Q[base_reg + index_reg << scale]*/
        else
                codegen_addbyte4(block, 0x48, 0x8b, 0x04 | ((dst_reg & 7) << 3), (scale << 6) | ((index_reg & 7) << 3) | (base_reg & 7)); /*MOV dst_reg, Q[base_reg + index_reg << scale]*/
}

void host_x86_MOV16_REG_BASE_OFFSET(codeblock_t *block, int dst_reg, int base_reg, int offset)
{
        if ((dst_reg & 8) || (base_reg & 8))
                fatal("host_x86_MOV16_REG_BASE_OFFSET reg & 8\n");

        if (offset >= -128 && offset < 127)
        {
                if (base_reg == REG_RSP)
                {
                        codegen_addbyte(block, 0x66);
                        codegen_addbyte4(block, 0x8b, 0x40 | base_reg | (dst_reg << 3), 0x24, offset);
                }
                else
                        codegen_addbyte4(block, 0x66, 0x8b, 0x40 | base_reg | (dst_reg << 3), offset);
        }
        else
                fatal("MOV16_REG_BASE_OFFSET - offset %i\n", offset);
}
void host_x86_MOV32_REG_BASE_OFFSET(codeblock_t *block, int dst_reg, int base_reg, int offset)
{
        if ((dst_reg & 8) || (base_reg & 8))
                fatal("host_x86_MOV32_REG_BASE_OFFSET reg & 8\n");

        if (offset >= -128 && offset < 127)
        {
                if (base_reg == REG_RSP)
                {
                        codegen_addbyte4(block, 0x8b, 0x40 | base_reg | (dst_reg << 3), 0x24, offset);
                }
                else
                        codegen_addbyte3(block, 0x8b, 0x40 | base_reg | (dst_reg << 3), offset);
        }
        else
                fatal("MOV32_REG_BASE_OFFSET - offset %i\n", offset);
}

void host_x86_MOV32_BASE_OFFSET_REG(codeblock_t *block, int base_reg, int offset, int src_reg)
{
        if ((src_reg & 8) || (base_reg & 8))
                fatal("host_x86_MOV64_BASE_OFFSET_REG reg & 8\n");

        if (offset >= -128 && offset < 127)
        {
                if (base_reg == REG_RSP)
                {
                        codegen_addbyte4(block, 0x89, 0x40 | base_reg | (src_reg << 3), 0x24, offset);
                }
                else
                        codegen_addbyte3(block, 0x89, 0x40 | base_reg | (src_reg << 3), offset);
        }
        else
                fatal("MOV32_BASE_OFFSET_REG - offset %i\n", offset);
}

void host_x86_MOV8_REG_IMM(codeblock_t *block, int reg, uint16_t imm_data)
{
        if (reg >= 8)
                fatal("host_x86_MOV8_REG_IMM reg >= 4\n");
        codegen_addbyte2(block, 0xb0 | reg, imm_data); /*MOV reg, imm_data*/
}
void host_x86_MOV16_REG_IMM(codeblock_t *block, int reg, uint16_t imm_data)
{
        if (reg & 8)
                fatal("host_x86_MOV16_REG_IMM reg & 8\n");
        codegen_addbyte2(block, 0x66, 0xb8 | (reg & 7)); /*MOV reg, imm_data*/
        codegen_addword(block, imm_data);
}
void host_x86_MOV32_REG_IMM(codeblock_t *block, int reg, uint32_t imm_data)
{
        if (reg & 8)
                fatal("host_x86_MOV32_REG_IMM reg & 8\n");
        codegen_addbyte(block, 0xb8 | (reg & 7)); /*MOV reg, imm_data*/
        codegen_addlong(block, imm_data);
}

void host_x86_MOV64_REG_IMM(codeblock_t *block, int reg, uint64_t imm_data)
{
        if (reg & 8)
        {
                codegen_addbyte2(block, 0x49, 0xb8 | (reg & 7)); /*MOVQ reg, imm_data*/
                codegen_addquad(block, imm_data);
        }
        else
        {
                codegen_addbyte2(block, 0x48, 0xb8 | (reg & 7)); /*MOVQ reg, imm_data*/
                codegen_addquad(block, imm_data);
        }
}

void host_x86_MOV8_REG_REG(codeblock_t *block, int dst_reg, int src_reg)
{
        if ((dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_MOV8_REG_REG - bad reg\n");

        codegen_addbyte2(block, 0x88, 0xc0 | (dst_reg & 7) | ((src_reg & 7) << 3));
}
void host_x86_MOV16_REG_REG(codeblock_t *block, int dst_reg, int src_reg)
{
        if ((dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_MOV16_REG_REG - bad reg\n");

        codegen_addbyte3(block, 0x66, 0x89, 0xc0 | (dst_reg & 7) | ((src_reg & 7) << 3));
}
void host_x86_MOV32_REG_REG(codeblock_t *block, int dst_reg, int src_reg)
{
        if ((dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_MOV32_REG_REG - bad reg\n");

        codegen_addbyte2(block, 0x89, 0xc0 | (dst_reg & 7) | ((src_reg & 7) << 3));
}

void host_x86_MOV32_STACK_IMM(codeblock_t *block, int32_t offset, uint32_t imm_data)
{
        if (!offset)
        {
                codegen_addbyte3(block, 0xc7, 0x04, 0x24); /*MOV [ESP], imm_data*/
                codegen_addlong(block, imm_data);
        }
        else if (offset >= -80 || offset < 0x80)
        {
                codegen_addbyte4(block, 0xc7, 0x44, 0x24, offset & 0xff); /*MOV offset[ESP], imm_data*/
                codegen_addlong(block, imm_data);
        }
        else
        {
                codegen_addbyte3(block, 0xc7, 0x84, 0x24); /*MOV offset[ESP], imm_data*/
                codegen_addlong(block, offset);
                codegen_addlong(block, imm_data);
        }
}

void host_x86_MOVD_BASE_INDEX_XREG(codeblock_t *block, int base_reg, int idx_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x7e, 0x04 | (src_reg << 3)); /*MOVD XMMx, [base_reg + idx_reg]*/
        codegen_addbyte(block, base_reg | (idx_reg << 3));
}
void host_x86_MOVD_REG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x7e, 0xc0 | dst_reg | (src_reg << 3));
}
void host_x86_MOVD_XREG_BASE_INDEX(codeblock_t *block, int dst_reg, int base_reg, int idx_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x6e, 0x04 | (dst_reg << 3)); /*MOVD XMMx, [base_reg + idx_reg]*/
        codegen_addbyte(block, base_reg | (idx_reg << 3));
}
void host_x86_MOVD_XREG_REG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x6e, 0xc0 | src_reg | (dst_reg << 3));
}

void host_x86_MOVQ_ABS_XREG(codeblock_t *block, void *p, int src_reg)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (src_reg & 8)
                fatal("host_x86_MOVQ_ABS_REG reg & 8\n");

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte4(block, 0x66, 0x0f, 0xd6, 0x45 | (src_reg << 3)); /*MOVQ offset[EBP], src_reg*/
                codegen_addbyte(block, offset);
        }
        else
        {
                if ((uintptr_t)p >> 32)
                        fatal("host_x86_MOVQ_ABS_REG - out of range %p\n", p);
                codegen_addbyte4(block, 0x66, 0x0f, 0xd6, 0x04 | (src_reg << 3)); /*MOVQ [p], src_reg*/
                codegen_addbyte(block, 0x25);
                codegen_addlong(block, (uint32_t)(uintptr_t)p);
        }
}
void host_x86_MOVQ_ABS_REG_REG_SHIFT_XREG(codeblock_t *block, uint32_t addr, int src_reg_a, int src_reg_b, int shift, int src_reg)
{
        if ((src_reg & 8) || (src_reg_a & 8) || (src_reg_b & 8))
                fatal("host_x86_MOVQ_ABS_REG_REG_SHIFT_REG - bad reg\n");
                
        codegen_addbyte3(block, 0x66, 0x0f, 0xd6); /*MOVQ addr[src_reg_a + src_reg_b << shift], XMMx*/
        if (addr < 0x80 || addr >= 0xffffff80)
        {
                codegen_addbyte3(block, 0x44 | (src_reg << 3), src_reg_a | (src_reg_b << 3) | (shift << 6), addr & 0xff);
        }
        else
        {
                codegen_addbyte2(block, 0x84 | (src_reg << 3), src_reg_a | (src_reg_b << 3) | (shift << 6));
                codegen_addlong(block, addr);
        }
}

void host_x86_MOVQ_BASE_INDEX_XREG(codeblock_t *block, int base_reg, int idx_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xd6, 0x04 | (src_reg << 3)); /*MOVD XMMx, [base_reg + idx_reg]*/
        codegen_addbyte(block, base_reg | (idx_reg << 3));
}
void host_x86_MOVQ_BASE_OFFSET_XREG(codeblock_t *block, int base_reg, int offset, int src_reg)
{
        if (offset >= -128 && offset < 127)
        {
                if (base_reg == REG_RSP)
                {
                        codegen_addbyte4(block, 0x66, 0x0f, 0xd6, 0x44 | (src_reg << 3)); /*MOVQ [RSP + offset], XMMx*/
                        codegen_addbyte2(block, 0x24, offset);
                }
                else
                {
                        codegen_addbyte4(block, 0x66, 0x0f, 0xd6, 0x40 | base_reg | (src_reg << 3)); /*MOVQ [base_reg + offset], XMMx*/
                        codegen_addbyte(block, offset);
                }
        }
        else
                fatal("MOVQ_BASE_OFFSET_XREG - offset %i\n", offset);
}

void host_x86_MOVQ_XREG_ABS(codeblock_t *block, int dst_reg, void *p)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

        if (dst_reg & 8)
                fatal("host_x86_MOVQ_REG_ABS reg & 8\n");

        if (offset >= -128 && offset < 127)
        {
                codegen_addbyte4(block, 0xf3, 0x0f, 0x7e, 0x45 | (dst_reg << 3)); /*MOVQ offset[EBP], src_reg*/
                codegen_addbyte(block, offset);
        }
        else
        {
                if ((uintptr_t)p >> 32)
                        fatal("host_x86_MOVQ_REG_ABS - out of range %p\n", p);
                codegen_addbyte4(block, 0xf3, 0x0f, 0x7e, 0x04 | (dst_reg << 3)); /*MOVQ [p], src_reg*/
                codegen_addbyte(block, 0x25);
                codegen_addlong(block, (uint32_t)(uintptr_t)p);
        }
}
void host_x86_MOVQ_XREG_ABS_REG_REG_SHIFT(codeblock_t *block, int dst_reg, uint32_t addr, int src_reg_a, int src_reg_b, int shift)
{
        if ((dst_reg & 8) || (src_reg_a & 8) || (src_reg_b & 8))
                fatal("host_x86_MOVQ_REG_ABS_REG_REG_SHIFT - bad reg\n");

        codegen_addbyte3(block, 0xf3, 0x0f, 0x7e); /*MOVQ XMMx, addr[src_reg_a + src_reg_b << shift]*/
        if (addr < 0x80 || addr >= 0xffffff80)
        {
                codegen_addbyte3(block, 0x44 | (dst_reg << 3), src_reg_a | (src_reg_b << 3) | (shift << 6), addr & 0xff);
        }
        else
        {
                codegen_addbyte2(block, 0x84 | (dst_reg << 3), src_reg_a | (src_reg_b << 3) | (shift << 6));
                codegen_addlong(block, addr);
        }
}
void host_x86_MOVQ_XREG_BASE_INDEX(codeblock_t *block, int dst_reg, int base_reg, int idx_reg)
{
        codegen_addbyte4(block, 0xf3, 0x0f, 0x7e, 0x04 | (dst_reg << 3)); /*MOVQ XMMx, [base_reg + idx_reg]*/
        codegen_addbyte(block, base_reg | (idx_reg << 3));
}
void host_x86_MOVQ_XREG_BASE_OFFSET(codeblock_t *block, int dst_reg, int base_reg, int offset)
{
        if (offset >= -128 && offset < 127)
        {
                if (base_reg == REG_ESP)
                {
                        codegen_addbyte4(block, 0xf3, 0x0f, 0x7e, 0x44 | (dst_reg << 3)); /*MOVQ XMMx, [ESP + offset]*/
                        codegen_addbyte2(block, 0x24, offset);
                }
                else
                {
                        codegen_addbyte4(block, 0xf3, 0x0f, 0x7e, 0x40 | base_reg | (dst_reg << 3)); /*MOVQ XMMx, [base_reg + offset]*/
                        codegen_addbyte(block, offset);
                }
        }
        else
                fatal("MOVQ_REG_BASE_OFFSET - offset %i\n", offset);
}

void host_x86_MOVQ_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf3, 0x0f, 0x7e, 0xc0 | src_reg | (dst_reg << 3)); /*MOVQ dst_reg, src_reg*/
}

void host_x86_MOVQ_REG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x48, 0x0f, 0x7e); /*MOVQ dst_reg, src_reg*/
        codegen_addbyte(block, 0xc0 | dst_reg | (src_reg << 3));
}
void host_x86_MOVQ_XREG_REG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x48, 0x0f, 0x6e); /*MOVQ dst_reg, src_reg*/
        codegen_addbyte(block, 0xc0 | src_reg | (dst_reg << 3));
}

void host_x86_MOVSX_REG_16_8(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xbe, 0xc0 | (dst_reg << 3) | src_reg); /*MOVSX dst_reg, src_reg*/
}
void host_x86_MOVSX_REG_32_8(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte3(block, 0x0f, 0xbe, 0xc0 | (dst_reg << 3) | src_reg); /*MOVSX dst_reg, src_reg*/
}
void host_x86_MOVSX_REG_32_16(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte3(block, 0x0f, 0xbf, 0xc0 | (dst_reg << 3) | src_reg); /*MOVSX dst_reg, src_reg*/
}

void host_x86_MOVZX_BASE_INDEX_32_8(codeblock_t *block, int dst_reg, int base_reg, int index_reg)
{
        if ((dst_reg & 8) || (base_reg & 8) | (index_reg & 8))
                fatal("host_x86_MOVZX_BASE_INDEX_32_8 reg & 8\n");
        codegen_addbyte4(block, 0x0f, 0xb6, 0x04 | (dst_reg << 3), (index_reg << 3) | base_reg);
}
void host_x86_MOVZX_BASE_INDEX_32_16(codeblock_t *block, int dst_reg, int base_reg, int index_reg)
{
        if ((dst_reg & 8) || (base_reg & 8) | (index_reg & 8))
                fatal("host_x86_MOVZX_BASE_INDEX_32_16 reg & 8\n");
        codegen_addbyte4(block, 0x0f, 0xb7, 0x04 | (dst_reg << 3), (index_reg << 3) | base_reg);
}

void host_x86_MOVZX_REG_16_8(codeblock_t *block, int dst_reg, int src_reg)
{
        if ((dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_MOVZX_REG_16_8 - bad reg\n");
        codegen_addbyte4(block, 0x66, 0x0f, 0xb6, 0xc0 | (dst_reg << 3) | src_reg); /*MOVZX dst_reg, src_reg*/
}
void host_x86_MOVZX_REG_32_8(codeblock_t *block, int dst_reg, int src_reg)
{
        if ((dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_MOVZX_REG_32_8 - bad reg\n");
        codegen_addbyte3(block, 0x0f, 0xb6, 0xc0 | (dst_reg << 3) | src_reg); /*MOVZX dst_reg, src_reg*/
}
void host_x86_MOVZX_REG_32_16(codeblock_t *block, int dst_reg, int src_reg)
{
        if ((dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_MOVZX_REG_16_8 - bad reg\n");
        codegen_addbyte3(block, 0x0f, 0xb7, 0xc0 | (dst_reg << 3) | src_reg); /*MOVZX dst_reg, src_reg*/
}

void host_x86_MOVZX_REG_ABS_32_8(codeblock_t *block, int dst_reg, void *p)
{
        int offset = (uintptr_t)p - (((uintptr_t)&cpu_state) + 128);

//        if (dst_reg & 8)
//                fatal("host_x86_MOVZX_REG_ABS_32_8 - bad reg\n");

        if (offset >= -128 && offset < 127)
        {
                if (dst_reg & 8)
                        codegen_addbyte(block, 0x44);
                codegen_addbyte4(block, 0x0f, 0xb6, 0x45 | ((dst_reg & 7) << 3), offset); /*MOVZX dst_reg, offset[RBP]*/
        }
        else
                fatal("host_x86_MOVZX_REG_ABS_32_8 - bad offset %i\n", offset);
}

void host_x86_MULSD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x0f, 0x59, 0xc0 | src_reg | (dst_reg << 3));
}

void host_x86_OR8_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint8_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_OR8_REG_IMM - dst_reg != src_reg\n");

        if (dst_reg == REG_EAX)
                codegen_addbyte2(block, 0x0c, imm_data); /*OR EAX, imm_data*/
        else
                codegen_addbyte3(block, 0x80, 0xc0 | RM_OP_OR | (dst_reg & 7), imm_data); /*OR dst_reg, imm_data*/
}
void host_x86_OR16_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint16_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_OR16_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte4(block, 0x66, 0x83, 0xc0 | RM_OP_OR | (dst_reg & 7), imm_data & 0xff); /*OR dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte2(block, 0x66, 0x0d); /*OR AX, imm_data*/
                codegen_addword(block, imm_data);
        }
        else
        {
                codegen_addbyte3(block, 0x66, 0x81, 0xc0 | RM_OP_OR | (dst_reg & 7)); /*OR dst_reg, imm_data*/
                codegen_addword(block, imm_data);
        }
}
void host_x86_OR32_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_OR32_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte3(block, 0x83, 0xc0 | RM_OP_OR | (dst_reg & 7), imm_data & 0xff); /*OR dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte(block, 0x0d); /*OR EAX, imm_data*/
                codegen_addlong(block, imm_data);
        }
        else
        {
                codegen_addbyte2(block, 0x81, 0xc0 | RM_OP_OR | (dst_reg & 7)); /*OR dst_reg, imm_data*/
                codegen_addlong(block, imm_data);
        }
}
void host_x86_OR8_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_OR8_REG_IMM - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x08, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*OR dst_reg, src_reg_b*/
}
void host_x86_OR16_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_OR16_REG_IMM - dst_reg != src_reg_a\n");

        codegen_addbyte3(block, 0x66, 0x09, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*OR dst_reg, src_reg_b*/
}
void host_x86_OR32_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_OR32_REG_IMM - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x09, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*OR dst_reg, src_reg_b*/
}

void host_x86_PACKSSWB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x63, 0xc0 | src_reg | (dst_reg << 3)); /*PACKSSWB dst_reg, src_reg*/
        codegen_addbyte4(block, 0x66, 0x0f, 0x70, 0xc0 | dst_reg | (dst_reg << 3)); /*PSHUFD dst_reg, dst_reg, 0x88 (move bits 64-95 to 32-63)*/
        codegen_addbyte(block, 0x88);
}
void host_x86_PACKSSDW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x6b, 0xc0 | src_reg | (dst_reg << 3)); /*PACKSSDW dst_reg, src_reg*/
        codegen_addbyte4(block, 0x66, 0x0f, 0x70, 0xc0 | dst_reg | (dst_reg << 3)); /*PSHUFD dst_reg, dst_reg, 0x88 (move bits 64-95 to 32-63)*/
        codegen_addbyte(block, 0x88);
}
void host_x86_PACKUSWB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x67, 0xc0 | src_reg | (dst_reg << 3)); /*PACKUSWB dst_reg, src_reg*/
        codegen_addbyte4(block, 0x66, 0x0f, 0x70, 0xc0 | dst_reg | (dst_reg << 3)); /*PSHUFD dst_reg, dst_reg, 0x88 (move bits 64-95 to 32-63)*/
        codegen_addbyte(block, 0x88);
}

void host_x86_PADDB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xfc, 0xc0 | src_reg | (dst_reg << 3)); /*PADDB dst_reg, src_reg*/
}
void host_x86_PADDW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xfd, 0xc0 | src_reg | (dst_reg << 3)); /*PADDW dst_reg, src_reg*/
}
void host_x86_PADDD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xfe, 0xc0 | src_reg | (dst_reg << 3)); /*PADDD dst_reg, src_reg*/
}
void host_x86_PADDSB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xec, 0xc0 | src_reg | (dst_reg << 3)); /*PADDSB dst_reg, src_reg*/
}
void host_x86_PADDSW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xed, 0xc0 | src_reg | (dst_reg << 3)); /*PADDSW dst_reg, src_reg*/
}
void host_x86_PADDUSB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xdc, 0xc0 | src_reg | (dst_reg << 3)); /*PADDUSB dst_reg, src_reg*/
}
void host_x86_PADDUSW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xdd, 0xc0 | src_reg | (dst_reg << 3)); /*PADDUSW dst_reg, src_reg*/
}

void host_x86_PAND_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xdb, 0xc0 | src_reg | (dst_reg << 3)); /*PAND dst_reg, src_reg*/
}
void host_x86_PANDN_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xdf, 0xc0 | src_reg | (dst_reg << 3)); /*PANDN dst_reg, src_reg*/
}
void host_x86_POR_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xeb, 0xc0 | src_reg | (dst_reg << 3)); /*POR dst_reg, src_reg*/
}
void host_x86_PXOR_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xef, 0xc0 | src_reg | (dst_reg << 3)); /*PXOR dst_reg, src_reg*/
}

void host_x86_PCMPEQB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x74, 0xc0 | src_reg | (dst_reg << 3)); /*PCMPEQB dst_reg, src_reg*/
}
void host_x86_PCMPEQW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x75, 0xc0 | src_reg | (dst_reg << 3)); /*PCMPEQW dst_reg, src_reg*/
}
void host_x86_PCMPEQD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x76, 0xc0 | src_reg | (dst_reg << 3)); /*PCMPEQD dst_reg, src_reg*/
}
void host_x86_PCMPGTB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x64, 0xc0 | src_reg | (dst_reg << 3)); /*PCMPGTB dst_reg, src_reg*/
}
void host_x86_PCMPGTW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x65, 0xc0 | src_reg | (dst_reg << 3)); /*PCMPGTW dst_reg, src_reg*/
}
void host_x86_PCMPGTD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x66, 0xc0 | src_reg | (dst_reg << 3)); /*PCMPGTD dst_reg, src_reg*/
}

void host_x86_PMADDWD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xf5, 0xc0 | src_reg | (dst_reg << 3)); /*PMULLW dst_reg, src_reg*/
}
void host_x86_PMULHW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xe5, 0xc0 | src_reg | (dst_reg << 3)); /*PMULLW dst_reg, src_reg*/
}
void host_x86_PMULLW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xd5, 0xc0 | src_reg | (dst_reg << 3)); /*PMULLW dst_reg, src_reg*/
}

void host_x86_PSLLW_XREG_IMM(codeblock_t *block, int dst_reg, int shift)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x71, 0xc0 | 0x30 | dst_reg); /*PSLLW dst_reg, imm*/
        codegen_addbyte(block, shift);
}
void host_x86_PSLLD_XREG_IMM(codeblock_t *block, int dst_reg, int shift)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x72, 0xc0 | 0x30 | dst_reg); /*PSLLD dst_reg, imm*/
        codegen_addbyte(block, shift);
}
void host_x86_PSLLQ_XREG_IMM(codeblock_t *block, int dst_reg, int shift)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x73, 0xc0 | 0x30 | dst_reg); /*PSLLD dst_reg, imm*/
        codegen_addbyte(block, shift);
}
void host_x86_PSRAW_XREG_IMM(codeblock_t *block, int dst_reg, int shift)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x71, 0xc0 | 0x20 | dst_reg); /*PSRAW dst_reg, imm*/
        codegen_addbyte(block, shift);
}
void host_x86_PSRAD_XREG_IMM(codeblock_t *block, int dst_reg, int shift)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x72, 0xc0 | 0x20 | dst_reg); /*PSRAD dst_reg, imm*/
        codegen_addbyte(block, shift);
}
void host_x86_PSRAQ_XREG_IMM(codeblock_t *block, int dst_reg, int shift)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x73, 0xc0 | 0x20 | dst_reg); /*PSRAD dst_reg, imm*/
        codegen_addbyte(block, shift);
}
void host_x86_PSRLW_XREG_IMM(codeblock_t *block, int dst_reg, int shift)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x71, 0xc0 | 0x10 | dst_reg); /*PSRLW dst_reg, imm*/
        codegen_addbyte(block, shift);
}
void host_x86_PSRLD_XREG_IMM(codeblock_t *block, int dst_reg, int shift)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x72, 0xc0 | 0x10 | dst_reg); /*PSRLD dst_reg, imm*/
        codegen_addbyte(block, shift);
}
void host_x86_PSRLQ_XREG_IMM(codeblock_t *block, int dst_reg, int shift)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x73, 0xc0 | 0x10 | dst_reg); /*PSRLD dst_reg, imm*/
        codegen_addbyte(block, shift);
}

void host_x86_PSUBB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xf8, 0xc0 | src_reg | (dst_reg << 3)); /*PADDB dst_reg, src_reg*/
}
void host_x86_PSUBW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xf9, 0xc0 | src_reg | (dst_reg << 3)); /*PADDW dst_reg, src_reg*/
}
void host_x86_PSUBD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xfa, 0xc0 | src_reg | (dst_reg << 3)); /*PADDD dst_reg, src_reg*/
}
void host_x86_PSUBSB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xe8, 0xc0 | src_reg | (dst_reg << 3)); /*PSUBSB dst_reg, src_reg*/
}
void host_x86_PSUBSW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xe9, 0xc0 | src_reg | (dst_reg << 3)); /*PSUBSW dst_reg, src_reg*/
}
void host_x86_PSUBUSB_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xd8, 0xc0 | src_reg | (dst_reg << 3)); /*PSUBUSB dst_reg, src_reg*/
}
void host_x86_PSUBUSW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0xd9, 0xc0 | src_reg | (dst_reg << 3)); /*PSUBUSW dst_reg, src_reg*/
}

void host_x86_PUNPCKHBW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x60, 0xc0 | src_reg | (dst_reg << 3)); /*PUNPCKLBW dst_reg, src_reg*/
        codegen_addbyte4(block, 0x66, 0x0f, 0x70, 0xc0 | dst_reg | (dst_reg << 3)); /*PSHUFD dst_reg, dst_reg, 0xee (move top 64-bits to low 64-bits)*/
        codegen_addbyte(block, 0xee);
}
void host_x86_PUNPCKHWD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x61, 0xc0 | src_reg | (dst_reg << 3)); /*PUNPCKLWD dst_reg, src_reg*/
        codegen_addbyte4(block, 0x66, 0x0f, 0x70, 0xc0 | dst_reg | (dst_reg << 3)); /*PSHUFD dst_reg, dst_reg, 0xee (move top 64-bits to low 64-bits)*/
        codegen_addbyte(block, 0xee);
}
void host_x86_PUNPCKHDQ_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x62, 0xc0 | src_reg | (dst_reg << 3)); /*PUNPCKLDQ dst_reg, src_reg*/
        codegen_addbyte4(block, 0x66, 0x0f, 0x70, 0xc0 | dst_reg | (dst_reg << 3)); /*PSHUFD dst_reg, dst_reg, 0xee (move top 64-bits to low 64-bits)*/
        codegen_addbyte(block, 0xee);
}
void host_x86_PUNPCKLBW_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x60, 0xc0 | src_reg | (dst_reg << 3)); /*PUNPCKLBW dst_reg, src_reg*/
}
void host_x86_PUNPCKLWD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x61, 0xc0 | src_reg | (dst_reg << 3)); /*PUNPCKLWD dst_reg, src_reg*/
}
void host_x86_PUNPCKLDQ_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0x66, 0x0f, 0x62, 0xc0 | src_reg | (dst_reg << 3)); /*PUNPCKLDQ dst_reg, src_reg*/
}

void host_x86_POP(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("POP reg & 8\n");
        codegen_addbyte(block, 0x58 | dst_reg); /*POP reg*/
}

void host_x86_PUSH(codeblock_t *block, int src_reg)
{
        if (src_reg & 8)
                fatal("PUSH reg & 8\n");
        codegen_addbyte(block, 0x50 | src_reg); /*PUSH reg*/
}

void host_x86_RET(codeblock_t *block)
{
        codegen_addbyte(block, 0xc3); /*RET*/
}

void host_x86_SAR8_CL(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("SAR8 CL & 8\n");
        codegen_addbyte2(block, 0xd2, 0xc0 | RM_OP_SAR | dst_reg); /*SAR dst_reg, CL*/
}
void host_x86_SAR16_CL(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("SAR16 CL & 8\n");
        codegen_addbyte3(block, 0x66, 0xd3, 0xc0 | RM_OP_SAR | dst_reg); /*SAR dst_reg, CL*/
}
void host_x86_SAR32_CL(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("SAR32 CL & 8\n");
        codegen_addbyte2(block, 0xd3, 0xc0 | RM_OP_SAR | dst_reg); /*SAR dst_reg, CL*/
}

void host_x86_SAR8_IMM(codeblock_t *block, int dst_reg, int shift)
{
        if (dst_reg & 8)
                fatal("SAR8 imm & 8\n");
        codegen_addbyte3(block, 0xc0, 0xc0 | RM_OP_SAR | dst_reg, shift); /*SAR dst_reg, shift*/
}
void host_x86_SAR16_IMM(codeblock_t *block, int dst_reg, int shift)
{
        if (dst_reg & 8)
                fatal("SAR16 imm & 8\n");
        codegen_addbyte4(block, 0x66, 0xc1, 0xc0 | RM_OP_SAR | dst_reg, shift); /*SAR dst_reg, shift*/
}
void host_x86_SAR32_IMM(codeblock_t *block, int dst_reg, int shift)
{
        if (dst_reg & 8)
                fatal("SAR32 imm & 8\n");
        codegen_addbyte3(block, 0xc1, 0xc0 | RM_OP_SAR | dst_reg, shift); /*SAR dst_reg, shift*/
}

void host_x86_SHL8_CL(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("SHL8 CL & 8\n");
        codegen_addbyte2(block, 0xd2, 0xc0 | RM_OP_SHL | dst_reg); /*SHL dst_reg, CL*/
}
void host_x86_SHL16_CL(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("SHL16 CL & 8\n");
        codegen_addbyte3(block, 0x66, 0xd3, 0xc0 | RM_OP_SHL | dst_reg); /*SHL dst_reg, CL*/
}
void host_x86_SHL32_CL(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("SHL32 CL & 8\n");
        codegen_addbyte2(block, 0xd3, 0xc0 | RM_OP_SHL | dst_reg); /*SHL dst_reg, CL*/
}

void host_x86_SHL8_IMM(codeblock_t *block, int dst_reg, int shift)
{
        if (dst_reg & 8)
                fatal("SHL8 imm & 8\n");
        codegen_addbyte3(block, 0xc0, 0xc0 | RM_OP_SHL | dst_reg, shift); /*SHL dst_reg, shift*/
}
void host_x86_SHL16_IMM(codeblock_t *block, int dst_reg, int shift)
{
        if (dst_reg & 8)
                fatal("SHL16 imm & 8\n");
        codegen_addbyte4(block, 0x66, 0xc1, 0xc0 | RM_OP_SHL | dst_reg, shift); /*SHL dst_reg, shift*/
}
void host_x86_SHL32_IMM(codeblock_t *block, int dst_reg, int shift)
{
        if (dst_reg & 8)
                fatal("SHL32 imm & 8\n");
        codegen_addbyte3(block, 0xc1, 0xc0 | RM_OP_SHL | dst_reg, shift); /*SHL dst_reg, shift*/
}

void host_x86_SHR8_CL(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("SHR8 CL & 8\n");
        codegen_addbyte2(block, 0xd2, 0xc0 | RM_OP_SHR | dst_reg); /*SHR dst_reg, CL*/
}
void host_x86_SHR16_CL(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("SHR16 CL & 8\n");
        codegen_addbyte3(block, 0x66, 0xd3, 0xc0 | RM_OP_SHR | dst_reg); /*SHR dst_reg, CL*/
}
void host_x86_SHR32_CL(codeblock_t *block, int dst_reg)
{
        if (dst_reg & 8)
                fatal("SHR32 CL & 8\n");
        codegen_addbyte2(block, 0xd3, 0xc0 | RM_OP_SHR | dst_reg); /*SHR dst_reg, CL*/
}

void host_x86_SHR8_IMM(codeblock_t *block, int dst_reg, int shift)
{
        if (dst_reg & 8)
                fatal("SHR8 imm & 8\n");
        codegen_addbyte3(block, 0xc0, 0xc0 | RM_OP_SHR | dst_reg, shift); /*SHR dst_reg, shift*/
}
void host_x86_SHR16_IMM(codeblock_t *block, int dst_reg, int shift)
{
        if (dst_reg & 8)
                fatal("SHR16 imm & 8\n");
        codegen_addbyte4(block, 0x66, 0xc1, 0xc0 | RM_OP_SHR | dst_reg, shift); /*SHR dst_reg, shift*/
}
void host_x86_SHR32_IMM(codeblock_t *block, int dst_reg, int shift)
{
        if (dst_reg & 8)
                fatal("SHR32 imm & 8\n");
        codegen_addbyte3(block, 0xc1, 0xc0 | RM_OP_SHR | dst_reg, shift); /*SHR dst_reg, shift*/
}

void host_x86_SQRTSD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x0f, 0x51, 0xc0 | src_reg | (dst_reg << 3)); /*SQRTSD dst_reg, src_reg*/
}

void host_x86_SUB8_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint8_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_SUB8_REG_IMM - dst_reg != src_reg\n");

        if (dst_reg == REG_EAX)
                codegen_addbyte2(block, 0x2c, imm_data); /*SUB EAX, imm_data*/
        else
                codegen_addbyte3(block, 0x80, 0xc0 | RM_OP_SUB | (dst_reg & 7), imm_data); /*SUB dst_reg, imm_data*/
}
void host_x86_SUB16_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint16_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_SUB16_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte4(block, 0x66, 0x83, 0xc0 | RM_OP_SUB | (dst_reg & 7), imm_data & 0xff); /*SUB dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte2(block, 0x66, 0x2d); /*SUB AX, imm_data*/
                codegen_addword(block, imm_data);
        }
        else
        {
                codegen_addbyte3(block, 0x66, 0x81, 0xc0 | RM_OP_SUB | (dst_reg & 7)); /*SUB dst_reg, imm_data*/
                codegen_addword(block, imm_data);
        }
}
void host_x86_SUB32_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_SUB32_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte3(block, 0x83, 0xc0 | RM_OP_SUB | (dst_reg & 7), imm_data & 0xff); /*SUB dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte(block, 0x2d); /*SUB EAX, imm_data*/
                codegen_addlong(block, imm_data);
        }
        else
        {
                codegen_addbyte2(block, 0x81, 0xc0 | RM_OP_SUB | (dst_reg & 7)); /*SUB dst_reg, imm_data*/
                codegen_addlong(block, imm_data);
        }
}
void host_x86_SUB64_REG_IMM(codeblock_t *block, int dst_reg, uint64_t imm_data)
{
        if (dst_reg & 8)
                fatal("host_x86_SUB64_REG_IMM - dst_reg & 8\n");

        if (is_imm8(imm_data))
                codegen_addbyte4(block, 0x48, 0x83, 0xc0 | RM_OP_SUB | (dst_reg & 7), imm_data & 0xff); /*SUB dst_reg, imm_data*/
        else
                fatal("SUB64_REG_IMM !is_imm8 %016llx\n", imm_data);
}
void host_x86_SUB8_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_SUB8_REG_REG - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x28, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*SUB dst_reg, src_reg_b*/
}
void host_x86_SUB16_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_SUB16_REG_REG - dst_reg != src_reg_a\n");

        codegen_addbyte3(block, 0x66, 0x29, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*SUB dst_reg, src_reg_b*/
}
void host_x86_SUB32_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_SUB32_REG_REG - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x29, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*SUB dst_reg, src_reg_b*/
}

void host_x86_SUBSD_XREG_XREG(codeblock_t *block, int dst_reg, int src_reg)
{
        codegen_addbyte4(block, 0xf2, 0x0f, 0x5c, 0xc0 | src_reg | (dst_reg << 3));
}

#define MODRM_MOD_REG(rm, reg) (0xc0 | reg | (rm << 3))

void host_x86_TEST8_REG(codeblock_t *block, int src_host_reg, int dst_host_reg)
{
        codegen_addbyte2(block, 0x84, MODRM_MOD_REG(dst_host_reg, src_host_reg)); /*TEST dst_host_reg, src_host_reg*/
}
void host_x86_TEST16_REG(codeblock_t *block, int src_host_reg, int dst_host_reg)
{
        codegen_addbyte3(block, 0x66, 0x85, MODRM_MOD_REG(dst_host_reg, src_host_reg)); /*TEST dst_host_reg, src_host_reg*/
}
void host_x86_TEST32_REG(codeblock_t *block, int src_reg, int dst_reg)
{
        if ((dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_TEST32_REG - bad reg\n");
        codegen_addbyte2(block, 0x85, MODRM_MOD_REG(dst_reg, src_reg)); /*TEST dst_host_reg, src_host_reg*/
}
void host_x86_TEST32_REG_IMM(codeblock_t *block, int dst_reg, uint32_t imm_data)
{
        if (dst_reg & 8)
                fatal("TEST32_REG_IMM reg & 8\n");
        if (dst_reg == REG_EAX)
        {
                codegen_addbyte(block, 0xa9); /*TEST EAX, imm_data*/
                codegen_addlong(block, imm_data);
        }
        else
        {
                codegen_addbyte2(block, 0xf7, 0xc0 | dst_reg); /*TEST dst_reg, imm_data*/
                codegen_addlong(block, imm_data);
        }
}

void host_x86_XOR8_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint8_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_XOR8_REG_IMM - dst_reg != src_reg\n");

        if (dst_reg == REG_EAX)
                codegen_addbyte2(block, 0x34, imm_data); /*XOR EAX, imm_data*/
        else
                codegen_addbyte3(block, 0x80, 0xc0 | RM_OP_XOR | (dst_reg & 7), imm_data); /*XOR dst_reg, imm_data*/
}
void host_x86_XOR16_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint16_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_XOR16_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte4(block, 0x66, 0x83, 0xc0 | RM_OP_XOR | (dst_reg & 7), imm_data & 0xff); /*XOR dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte2(block, 0x66, 0x35); /*XOR AX, imm_data*/
                codegen_addword(block, imm_data);
        }
        else
        {
                codegen_addbyte3(block, 0x66, 0x81, 0xc0 | RM_OP_XOR | (dst_reg & 7)); /*XOR dst_reg, imm_data*/
                codegen_addword(block, imm_data);
        }
}
void host_x86_XOR32_REG_IMM(codeblock_t *block, int dst_reg, int src_reg, uint32_t imm_data)
{
        if (dst_reg != src_reg || (dst_reg & 8) || (src_reg & 8))
                fatal("host_x86_XOR32_REG_IMM - dst_reg != src_reg\n");

        if (is_imm8(imm_data))
                codegen_addbyte3(block, 0x83, 0xc0 | RM_OP_XOR | (dst_reg & 7), imm_data & 0xff); /*XOR dst_reg, imm_data*/
        else if (dst_reg == REG_EAX)
        {
                codegen_addbyte(block, 0x35); /*XOR EAX, imm_data*/
                codegen_addlong(block, imm_data);
        }
        else
        {
                codegen_addbyte2(block, 0x81, 0xc0 | RM_OP_XOR | (dst_reg & 7)); /*XOR dst_reg, imm_data*/
                codegen_addlong(block, imm_data);
        }
}
void host_x86_XOR8_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_XOR8_REG_IMM - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x30, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*XOR dst_reg, src_reg_b*/
}
void host_x86_XOR16_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_XOR16_REG_IMM - dst_reg != src_reg_a\n");

        codegen_addbyte3(block, 0x66, 0x31, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*XOR dst_reg, src_reg_b*/
}
void host_x86_XOR32_REG_REG(codeblock_t *block, int dst_reg, int src_reg_a, int src_reg_b)
{
        if (dst_reg != src_reg_a || (dst_reg & 8) || (src_reg_b & 8))
                fatal("host_x86_XOR32_REG_IMM - dst_reg != src_reg_a\n");

        codegen_addbyte2(block, 0x31, 0xc0 | (dst_reg & 7) | ((src_reg_b & 7) << 3)); /*XOR dst_reg, src_reg_b*/
}

#endif