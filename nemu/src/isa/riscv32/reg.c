/***************************************************************************************
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
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

#define LK_show_reg(name,data) printf("%8s 0x%08x %d\n",name,data,data)
#define LK_show_2(name) LK_show_reg(str(name),cpu.name)

void isa_reg_display() {
  for(int i=0;i<32;++i) 
    LK_show_reg(regs[i],gpr(i));
  LK_show_reg("pc",cpu.pc);

  return;
}

word_t isa_reg_str2val(const char *s, bool *success) {

  for(int i=0;i<32;++i){

    if(strcmp(s+1,regs[i])==0){
      *success=true;
      return gpr(i);
    }

  }
  if(strcmp(s+1,"pc")==0){
    *success=true;
    return cpu.pc;
  }
  *success=false;
  return 0;
}

