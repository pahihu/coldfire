/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*  & Matt Minnis                 */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(monitor);

#include "elf-include.h"

UBYTE *elfbuffer=NULL;
ULONG elfsize=0;
struct Elf32_Ehdr *elfheader=NULL;
struct Elf32_Phdr *elfsegments=NULL;
struct Elf32_Shdr *elfsections=NULL;
struct Elf32_Sym  *elfsymtab=NULL;

ULONG elfsymtabcount=0;
struct Elf32_Rela *elfrela=NULL;
ULONG elfrelacount=0;
char *sectionnames=NULL;
char *strtab=NULL;

char *sectiontypes[]=
{
  "SHT_NULL",
  "SHT_PROGBITS",
  "SHT_SYMTAB",
  "SHT_STRTAB",
  "SHT_RELA",
  "SHT_HASH",
  "SHT_DYNAMIC",
  "SHT_NOTE",
  "SHT_NOBITS",
  "SHT_REL",
  "SHT_SHLIB",
  "SHT_DYNSYM"
};

LONG proglength=0;
UBYTE *progbuffer;
ULONG baseaddress=0x10000;
int verbosemode=0;

LONG get_section_addr(LONG section)
{
  LONG i, addr=0;

  for(i=0; i<ECW(elfheader->e_shnum); i++)
  {
    if (i==section)
      return addr;

    if (ECL(elfsections[i].sh_type)==SHT_PROGBITS)
      addr+=ECL(elfsections[i].sh_size);
  }

  return -1;
}

LONG get_sym_addr(char *symbol)
{
  ULONG i, symsection=-1, adr=0;

  for(i=0; i<elfsymtabcount; i++)
  {
    if (strcmp(&strtab[ECL(elfsymtab[i].st_name)], symbol)==0)
    {
      symsection=ECW(elfsymtab[i].st_shndx);
      break;
    }
  }

  if (symsection!=-1)
  {

    for(i=0; i<elfsymtabcount; i++)
    {
      if (ECW(elfsymtab[i].st_shndx)==symsection)
      {
        if (strcmp(&strtab[ECL(elfsymtab[i].st_name)], symbol)==0)
        {
          adr+=get_section_addr(ECW(elfsymtab[i].st_shndx));
          return adr;
        }
        adr+=ECL(elfsymtab[i].st_size);
      }
    }
  }

  return -1;
}

void scan_sections(void)
{
  LONG i;

  for(i=0; i<ECW(elfheader->e_shnum); i++)
  {
    if (ECL(elfsections[i].sh_type)==SHT_STRTAB && i==ECW(elfheader->e_shstrndx))
    {
      sectionnames=elfbuffer+ECL(elfsections[i].sh_offset);
    }
    else if (ECL(elfsections[i].sh_type)==SHT_STRTAB)
    {
      strtab=elfbuffer+ECL(elfsections[i].sh_offset);
    }
    else if (ECL(elfsections[i].sh_type)==SHT_SYMTAB)
    {
      elfsymtab=(struct Elf32_Sym *) (elfbuffer+ECL(elfsections[i].sh_offset));
      elfsymtabcount=ECL(elfsections[i].sh_size)/sizeof(struct Elf32_Sym);
    }
    else if (ECL(elfsections[i].sh_type)==SHT_PROGBITS)
    {
      proglength+=ECL(elfsections[i].sh_size);
    }
    else if (ECL(elfsections[i].sh_type)==SHT_RELA)
    {
      elfrela=(struct Elf32_Rela *) (elfbuffer+ECL(elfsections[i].sh_offset));
      elfrelacount=ECL(elfsections[i].sh_size)/sizeof(struct Elf32_Rela);
    }
  }
}

extern char *memory_ram;
void load_program(void)
{
  LONG i,j;
  UBYTE *buf; /*Will point to memory_ram; */
  s32 Offset=baseaddress;

  
  for(i=0; i<ECW(elfheader->e_shnum); i++)
  {
    if (ECL(elfsections[i].sh_type)==SHT_PROGBITS)
    {
      if (ECL(elfsections[i].sh_size)>0)
      {      	 
        if ((strcmp(&sectionnames[ECL(elfsections[i].sh_name)],".text")==0))  /*Find the executable section */
          {            
             j=0;
             buf=&elfbuffer[ECL(elfsections[i].sh_offset)];
             while (j<ECL(elfsections[i].sh_size))
               {
               	Memory_StorByte(Offset++, (char)*buf++);
               	j++;
               }
             printf("0x%x bytes loaded.\n",j);
          }
      }

    }
  }
}

int read_then_get_elf(char *Str)
{
  FILE *elffile; 
  char *infile=""; 

  verbosemode=1;
  if (1)
  {
  infile=Str;
  if ((elffile=fopen(infile, "rb")))
  {
    fseek(elffile, 0, SEEK_END);
    elfsize=ftell(elffile);
    fseek(elffile, 0, SEEK_SET);

    if ((elfbuffer=malloc(elfsize)))
    {
      fread(elfbuffer, elfsize, 1, elffile);
      elfheader=(struct Elf32_Ehdr *) elfbuffer;

      if (verbosemode==1) printf("ELF Loader v0.1a:\n");
      printf("ELF file size: %d bytes.\n", elfsize);

      if (elfheader->e_ident[EI_MAG0]==0x7F &&
          elfheader->e_ident[EI_MAG1]=='E' &&
          elfheader->e_ident[EI_MAG2]=='L' &&
          elfheader->e_ident[EI_MAG3]=='F' &&
          elfheader->e_ident[EI_CLASS]==1 &&
          elfheader->e_ident[EI_DATA]==2 &&
          elfheader->e_ident[EI_VERSION]==EV_CURRENT &&
          ECW(elfheader->e_machine)==EM_68K &&
          ECL(elfheader->e_version)==EV_CURRENT)  /*Make sure this is appropriate for us to run */
          
      {
        printf("Identified as Motorola 68k 32Bit ELF file, version: %d.\n",ECL(elfheader->e_version));

        if ((ECW(elfheader->e_type)==ET_REL) || (ECW(elfheader->e_type)==ET_EXEC))
        {
          if (ECW(elfheader->e_shentsize)==sizeof(struct Elf32_Shdr))
          {
            ULONG mainadr;
            printf("Section size is Ok. There are %d sections.\n", ECW(elfheader->e_shnum));
            elfsections=(struct Elf32_Shdr *) (elfbuffer+ECL(elfheader->e_shoff));

            scan_sections();
            if ((progbuffer=malloc(proglength)))
            {
              printf("Using base address $%x.\n", baseaddress);

              load_program();

              mainadr=get_sym_addr("main")+baseaddress;
              free(progbuffer);
            }
            else printf("Error: not enough memory!\n");
          }
        }
      }

      free(elfbuffer);
    }
    else printf("Error: not enough memory!\n");

    fclose(elffile);
  }
}
  return 0;
}

int Monitor_DE(int argc, char **argv)
{
	if(argc != 2) return Monitor_show_help("de");

	read_then_get_elf(argv[1]);
	printf("\n");
	return 1;
}
