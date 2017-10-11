#ifndef _PACOS_IAPN_H
#define _PACOS_IAPN_H



#include "pal.h"
#include "linklist.h"
#include "memory.h"
#include "cli.h"
#include "if.h"


#define MAX_MEG          20000

#define SUCCESS 0
#define ERROR 1

#define STACK_INIT_SIZE 2001
#define MAX_CAP_NUM     2000
#define STACK_INCREMENT 20



#ifndef PAL_HW_ALEN
#define PAL_HW_ALEN 6
#endif

enum MEGTYPE
{
  PW = 1,
  LSP = 2,
  Section = 3 ,
  NOMEGTYPE = 4
};//czh

enum Mep_Class
{
  SOURCE = 1,
  DEST = 2,
  BIDI = 3,
  No_Mep_Class= 4
};

enum  Mep_State
{
  Mep_Up = 1,
  Mep_Down = 2
};

enum  Mip_State
{
 Mip_Up = 1,
 Mip_Down = 2
}; //czh

enum Mip_En
{
 Mip_Enable = 1 ,
 Mip_Disable = 2
};

enum Cc_En
{
 Cc_Enable = 1 ,
 Cc_Disable = 2
};
enum Send_Timer
{
 Fast_Default = 1,
 Fast_10_Ms = 2,
 Fast_100_Ms = 3 ,
 Fast_1_S =4 ,
 Fast_10_S =5,
 Fast_1_M = 6,
 Fast_10_M = 7

}; //czh

enum Pro_Active_Lm
{
Lm_Enable = 1,
Lm_Disable  =2
};

typedef struct MEG
{
  unsigned int Meg_Index;
  unsigned int Meg_Id;
  enum MEGTYPE Meg_Type;
  enum Mep_Class iLocal_Mep_Class;
  enum Mep_State iLocal_Mep_State;
  unsigned int Local_Mep_Id;
  enum Mep_Class iPeer_Mep_Class;
  unsigned int Peer_Mep_Id;
  enum Mip_En iMip_Enable;
  enum Mip_State iMip_State;
  unsigned int Mip_Id;
  unsigned int Object_Key;
  enum Cc_En iCc_Enable;
  enum Send_Timer iSend_Timer;
  unsigned int Loss_Pkt_Thr ;
  enum Pro_Active_Lm iPro_Active_Lm;
  struct MEG *next,*prev;
  struct OAM_TOOL  *Oam_Toolp;

}*pMEG;  //czh

enum Flag_Rdi
{
Rdi_No= 0,
Rdi_Yes  =1
};
enum Flag_Period
{
Period_Invalid=0,
Period_Default=1,
Period_10ms=2,
Period_100ms=3,
Period_1s=4,
Period_10s=5,
Period_1min=6,
Period_10min=7
};

enum Opcode_Lb
{
Opcode_Lbr =2,
Opcode_Lbm =3
};

enum Lb_Tlv
{
Lb_Tlv_Data =1,
Lb_Tlv_Test =2
};

enum Opcode_Lt
{
Opcode_Lt_Ltr=4,
Opcode_Lt_Ltm =5
};
enum Flag_Hw
{
Hw_No=0,
Hw_Yes =1
};


enum Opcode_Lm
{
Opcode_Lmr=42,
Opcode_Lmm=43
};

enum Opcode_Dm
{
Opcode_Dmr=46,
Opcode_Dmm=47
};

enum LM_STATE
{
Lm_Off =0,
Lm_On  =1
};

typedef struct OAM_TOOL
{
  struct OAM_TOOL_CC *Oam_Tool_Ccp;
  struct OAM_TOOL_LB *Oam_Tool_Lbp;
  struct OAM_TOOL_LT *Oam_Tool_Ltp;
  struct OAM_TOOL_DM *Oam_Tool_Dmp;
  struct OAM_TOOL_LM *Oam_Tool_Lmp;
}*pOAM_TOOL;



typedef struct OAM_TOOL_CC
{
  int Mel;
  int Ver;
  int Opcode;
  enum Flag_Rdi iFlag_Rdi;
  enum Flag_Period iFlag_Period;
  long int  Cc_Seq_Num;
  int Mep_Id;
  int  Meg_Id;
  long int  Tx_Fcf;
  long int  Rx_Fcb;
  long int  Tx_Fcb;
}*pOAM_TOOL_CC;





typedef struct OAM_TOOL_LB
{
  int  Mel;
  int  Ver;
  enum Opcode_Lb iOpcode_Lb;
  int  Flag;
  int  Tlv_Offset;
  long int  Lb_Seq_Num;
  enum Lb_Tlv iIb_Tlv;
  int Ttl;
  int Repeat_Num ;
  int Packet_Size;
  int Time_Out;
}*pOAM_TOOL_LB;




typedef struct OAM_TOOL_LT
{
  int  Mel;
  int Ver;
  enum Opcode_Lt iOpcode_Lt;
  enum Flag_Hw  iFlag_Hw;
  int Tlv_Offset ;
  long int  Lt_Trans_Id;
  int  Ttl;
  char Da_Mac;
  char Sa_Mac;
  int Packet_Size;
  int Time_Out;
 }*pOAM_TOOL_LT;






typedef struct OAM_TOOL_DM
{
  int  Mel;
  int  Ver;
  enum Opcode_Dm iOpcode_Dm;
  int  Flag;
  int  Tlv_Offset;
  long int Tx_Timer_1;
  long int  Rx_Timer;
  long int Tx_Timer_2;
  int  Ttl;
  char Da_Mac;
  char Sa_Mac;
  int Repeat_Num ;
  int Dm_Period;
  int Result;
}*pOAM_TOOL_DM;

typedef struct OAM_TOOL_LM
{
  int  Mel;
  int  Ver;
  enum Opcode_Lm iOpcode_Lm;
  int Flag;
  int Tlv_Offset;
  long int  Tx_Fcf;
  long int  Rx_Fcf;
  long int  Tx_Fcb;
  int Repeat_Num ;
  int Lm_Period;
  enum LM_STATE Lm_State;
  int Result;
}*pOAM_TOOL_LM;



void meg_master_init(struct meg_master *megg, struct lib_globals *zg);




int init_stack( struct stack_index *s);

struct stack_index * destory_stack( struct stack_index *s);
struct stack_index * clear_stack( struct stack_index *s);
int push_stack( struct stack_index *s,int e);
int pop_stack( struct stack_index *s );
int get_top( struct stack_index *s, int *e);



#endif

