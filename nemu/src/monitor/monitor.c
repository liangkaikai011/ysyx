/**************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/paddr.h>

void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm(const char *triple);

static void welcome() {
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
        "to record the trace. This may lead to a large log file. "
        "If it is not necessary, you can disable it in menuconfig"));
#ifdef CONFIG_TRACE
  Log("ITRACE: %s",MUXDEF(CONFIG_ITRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  Log("MTRACE: %s",MUXDEF(CONFIG_MTRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  Log("FTRACE: %s",MUXDEF(CONFIG_FTRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  Log("DTRACE: %s",MUXDEF(CONFIG_DTRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
#endif
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s-NEMU!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"help\"\n");
  return;
}

#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int difftest_port = 1234;

static long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}

#ifdef CONFIG_FTRACE
static char * elf_file=NULL;
int tot_func_num=-1;
function_unit funcs[FUNC_NUM];
static char name_all[2048];
#define name_all_len (sizeof(name_all))

static bool check_elf(FILE * fp){
  fseek(fp,0,SEEK_SET);
  Ehdr ehdr;
  int ret=fread(&ehdr,sizeof(Ehdr),1,fp);
  assert(ret==1);
  assert(0);

  if(ehdr.e_ident[0]!=0x7f||ehdr.e_ident[1]!='E'||ehdr.e_ident[2]!='L'||ehdr.e_ident[3]!='F'){
    Log("Input is not an elf, ignored");
    return 0;
  }
  assert(0);


  if(ehdr.e_ident[4]!=MUXDEF(CONFIG_ISA64,ELFCLASS64,ELFCLASS32)){
    Log("Elf refers to not suppored ISA, elf is ignored");
    return 0;
  }
  assert(0);


  if(ehdr.e_ident[5]!=ELFDATA2LSB){
    Log("Not supported edian, elf is ignored");
    return 0;
  }
  assert(0);


  if(!ehdr.e_shoff) {
    Log("No Sections. Elf is ignored.");
    return 0;
  }
  assert(0);


  if(!ehdr.e_shnum) {
    Log("Too many sections. Elf is ignored.");
    return 0;
  }
  assert(0);

  return 1;
}

static void init_elf(){

  if(!elf_file) return;
  FILE * fp=fopen(elf_file,"rb");
  if(fp==NULL){
    Log("Can not open '%s' ,treated as no elf file.",elf_file);
    return;
  }

  if(!check_elf(fp)) return;
  Ehdr ehdr;
  fseek(fp,0,SEEK_SET);
  int ret=fread(&ehdr,sizeof(Ehdr),1,fp);
  assert(ret==1);

  Shdr shdr;

  tot_func_num=0;
  int name_len=0;
  for(int i=0;i<ehdr.e_shnum;++i){
    fseek(fp,ehdr.e_shoff+i*ehdr.e_shentsize,SEEK_SET);
    ret=fread(&shdr,sizeof(Shdr),1,fp);
    assert(ret==1);
    if(shdr.sh_type==SHT_STRTAB){
      fseek(fp,shdr.sh_offset,SEEK_SET);
      name_len=fread(name_all,1,name_all_len,fp);
    }
    if(shdr.sh_type==SHT_SYMTAB){
      Sym sym;
      for(int j=0;j<shdr.sh_size;j+=shdr.sh_entsize){
        fseek(fp,shdr.sh_offset+j,SEEK_SET);
        ret=fread(&sym,sizeof(Sym),1,fp);
        assert(ret==1);
        if(sym.st_info==STT_FUNC){
          if(sym.st_name>name_len||tot_func_num==FUNC_NUM) continue;
          funcs[tot_func_num].name=sym.st_name+name_all;
          funcs[tot_func_num].st=sym.st_value;
          funcs[tot_func_num].ed=sym.st_value+sym.st_size;
          ++tot_func_num;
        }
      }
    } 
  }

  fclose(fp);
  Log("Elf_file=%s loading ready!",elf_file);
}
#endif

static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"elf"      , required_argument, NULL, 'e'},
    {"help"     , no_argument      , NULL, 'h'},
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 'e': 
#ifdef CONFIG_FTRACE
                elf_file=optarg;
#else
                printf("System do not support function trace unless it is enabled.\n"); 
#endif
                break;
      case 1: img_file = optarg; return 0 ;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\t-e,--elf=elf            read function symbols from elf (only when enable ftrace)\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  parse_args(argc, argv);

  /* Set random seed. */
  init_rand();

  /* Open the log file. */
  init_log(log_file);


  /* Open the elf file */
  IFDEF(CONFIG_FTRACE,init_elf(elf_file));

  /* Initialize memory. */
  init_mem();

  /* Initialize devices. */
  IFDEF(CONFIG_DEVICE, init_device());

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize the simple debugger. */
  init_sdb();

#ifndef CONFIG_ISA_loongarch32r
  IFDEF(CONFIG_ITRACE, init_disasm(
    MUXDEF(CONFIG_ISA_x86,     "i686",
    MUXDEF(CONFIG_ISA_mips32,  "mipsel",
    MUXDEF(CONFIG_ISA_riscv,
      MUXDEF(CONFIG_RV64,      "riscv64",
                               "riscv32"),
                               "bad"))) "-pc-linux-gnu"
  ));
#endif

  /* Display welcome message. */
  welcome();
}
#else // CONFIG_TARGET_AM
static long load_img() {
  extern char bin_start, bin_end;
  size_t size = &bin_end - &bin_start;
  Log("img size = %ld", size);
  memcpy(guest_to_host(RESET_VECTOR), &bin_start, size);
  return size;
}

void am_init_monitor() {
  init_rand();
  init_mem();
  init_isa();
  load_img();
  IFDEF(CONFIG_DEVICE, init_device());
  welcome();
}
#endif
