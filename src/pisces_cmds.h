#ifndef __PISCES_CMDS_H__
#define __PISCES_CMDS_H__

#define PISCES_CTRL_IPI_VECTOR 220

#define ENCLAVE_CMD_ADD_CPU      100
#define ENCLAVE_CMD_ADD_MEM      101


struct ctrl_cmd {
    u64 cmd;
    u32 data_len;
    u8 data[0];
} __attribute__((packed));


struct ctrl_resp {
    u64 status;
    u32 data_len;
    u8 data[0];
} __attribute__((packed));


struct ctrl_cmd_buf {
  union {
    u64 flags;
    struct {
      u8 ready          : 1;   // Flag set by enclave OS, after channel is init'd
      u8 active         : 1;   // Set when a command is in progress
      u8 completed      : 1;   // Set by enclave OS when command has been handled
      u32 rsvd          : 29;
    } __attribute__((packed));
  } __attribute__((packed));

  union {
    struct ctrl_cmd cmd;
    struct ctrl_resp resp;
  } __attribute__((packed));

} __attribute__((packed));



struct cmd_cpu_add {
    struct ctrl_cmd hdr;
    u64 apic_id;
} __attribute__((packed));


struct cmd_mem_add {
    struct ctrl_cmd hdr;
    u64 phys_addr;
    u64 size;
} __attribute__((packed));



#endif
