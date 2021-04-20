#ifndef __ELF_H
#define __ELF_H

typedef char BYTE;
typedef unsigned char UBYTE;
typedef s16 WORD;
typedef u16 UWORD;
typedef s32 LONG;
typedef u32 ULONG;
typedef char * STRPTR;

/*#define SUN
#define BIGENDIAN*/

#ifdef BIGENDIAN
  #define ECW(i) (i)
  #define ECL(i) (i)
#else
  #define ECW(i) ( (((i)&0xFF)<<8) | (((i)>>8)&0xff) )
  #define ECL(i) ( (((i)&0xFF)<<24) | (((i)<<8)&0xff0000) | (((i)>>8)&0xff00) | ((i)>>24))
#endif


#define EI_NIDENT 16

struct Elf32_Ehdr
{
  UBYTE e_ident[EI_NIDENT];
  UWORD e_type;
  UWORD e_machine;
  ULONG e_version;
  ULONG e_entry;
  ULONG e_phoff;
  ULONG e_shoff;
  ULONG e_flags;
  UWORD e_ehsize;
  UWORD e_phentsize;
  UWORD e_phnum;
  UWORD e_shentsize;
  UWORD e_shnum;
  UWORD e_shstrndx;
};

/* --- e_indent indexes  ---*/
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_PAD 7

/* --- EI_CLASS  ---*/
#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

/* --- EI_DATA  --- */
#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

/* --- e_type  --- */
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

/* --- e_version  --- */
#define EV_NONE 0
#define EV_CURRENT 1

/* --- e_machine  --- */
#define EM_NONE 0
#define EM_M32 1
#define EM_SPARC 2
#define EM_386 3
#define EM_68K 4
#define EM_88K 5
#define EM_860 7
#define EM_MIPS 8
#define EM_POWERPC 20


struct Elf32_Shdr
{
  ULONG sh_name;
  ULONG sh_type;
  ULONG sh_flags;
  ULONG sh_addr;
  ULONG sh_offset;
  ULONG sh_size;
  ULONG sh_link;
  ULONG sh_info;
  ULONG sh_addralign;
  ULONG sh_entsize;
};

/* --- special sections indexes  --- */
#define SHN_UNDEF  0
#define SHN_ABS    0xfff1
#define SHN_COMMON 0xfff2

/* --- sh_type  --- */
#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_RELA     4
#define SHT_HASH     5
#define SHT_DYNAMIC  6
#define SHT_NOTE     7
#define SHT_NOBITS   8
#define SHT_REL      9
#define SHT_SHLIB   10
#define SHT_DYNSYM  11

/* --- sh_flags  --- */
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4


struct Elf32_Sym
{
  ULONG st_name;
  ULONG st_value;
  ULONG st_size;
  UBYTE st_info;
  UBYTE st_other;
  UWORD st_shndx;
};

/* --- st_info  --- */
#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

/* --- ST_BIND  --- */
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2

/* --- ST_TYPE  --- */
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4


struct Elf32_Rel
{
  ULONG r_offset;
  ULONG r_info;
};

struct Elf32_Rela
{
  ULONG r_offset;
  ULONG r_info;
  ULONG r_addend;
};

/* --- r_info  --- */
#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((UBYTE)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(UBYTE)(t))



struct Elf32_Phdr
{
  ULONG p_type;
  ULONG p_offset;
  ULONG p_vaddr;
  ULONG p_paddr;
  ULONG p_filesz;
  ULONG p_memsz;
  ULONG p_flags;
  ULONG p_align;
};

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

#endif

