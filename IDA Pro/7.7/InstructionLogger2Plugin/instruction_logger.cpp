#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <dbg.hpp>

//--------------------------------------------------------------------------
struct instruction_logger_plugin_t : public plugmod_t
{
  processor_t& ph;
  instruction_logger_plugin_t(processor_t& _ph) : ph(_ph) {}
  virtual bool idaapi run(size_t) override;
};

struct user_data_t {
  FILE *logFile;
};

void strToLower(qstring in, char* out) {
  if (in.length() > QMAXPATH)
  {
    return;
  }
  std::transform(in.begin(), in.end(), out,
    [](unsigned char c) { return std::tolower(c); });
}

bool isKnownModule(qstring mod_name) {
  char Path[QMAXPATH+1];
  memset(Path, 0, sizeof(Path));
  strToLower(mod_name, Path);
  return (strcmp(Path, "ntdll.dll") == 0
    || strcmp(Path, "ntdll32.dll") == 0
    || strcmp(Path, "win32u.dll") == 0
    || strcmp(Path, "c:\\windows\\system32\\kernelbase.dll") == 0
    || strcmp(Path, "c:\\windows\\system32\\msvcrt.dll") == 0
    || strcmp(Path, "c:\\windows\\system32\\winbrand.dll") == 0
    || strcmp(Path, "c:\\windows\\system32\\wow64cpu.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\kernel32.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\kernelbase.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\win32u.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\rpcrt4.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\sechost.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\user32.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\msvcrt.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\ddraw.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\ucrtbase.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\d3d8.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\resampledmo.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\uxtheme.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\gdi32full.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\gdi32.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\combase.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\bcryptprimitives.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\ole32.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\advapi32.dll") == 0
    || strcmp(Path, "c:\\windows\\syswow64\\dmusic.dll") == 0
  );
}

struct user_data_t puser_data;
std::set<ea_t> whitelist;
std::set<ea_t> nearWhitelist;
std::set<ea_t> displWhitelist;

void loadWhiteList() {
  FILE* whiteFile;
  ssize_t length;
  qstring buf;
  qstrvec_t vec;
  whiteFile = qfopen("C:\\Users\\Sphere\\Desktop\\04_myaquaticlife\\logfile_unique.txt", "r");
  while ((length = qgetline(&buf, whiteFile)) > 0) {
    vec.clear();
    buf.split(&vec, ",");
    // vec[0] => thread
    // vec[1] => ip
    // vec[2] => module
    char* p;
    unsigned long num;
    msg("Parsing %s\n", vec[1].c_str());
    num = strtoul(vec[1].c_str(), &p, 16);
    if (*p == 0) {
      whitelist.insert((ea_t)num);
    }
    else {
      msg("Ignored invalid value\n");
    }
  }
}

bool isWhiteListed(ea_t value) {
  return (std::find(whitelist.begin(), whitelist.end(), value) != whitelist.end());
}

bool isNearCallWhiteListed(ea_t value) {
  return (std::find(nearWhitelist.begin(), nearWhitelist.end(), value) != nearWhitelist.end());
}

bool isDisplCallWhiteListed(ea_t value) {
  return (std::find(displWhitelist.begin(), displWhitelist.end(), value) != displWhitelist.end());
}

//--------------------------------------------------------------------------
ssize_t idaapi hook_callback(void* user_data, int notification_code, va_list va) {

  switch (notification_code) {
    case dbg_process_start: {
      msg("hook notification: Process Started\n");
      
      //request_clear_trace();
      //request_set_step_trace_options(ST_ALREADY_LOGGED);
      puser_data.logFile = qfopen("C:\\Users\\Sphere\\Desktop\\04_myaquaticlife\\logfile2.txt", "w");
      qfprintf(puser_data.logFile, "thread,ip,module\n");
      break;
    }
    case dbg_process_exit: {
      msg("hook notification: Process Exited\n");
      qflush(puser_data.logFile);
      qfclose(puser_data.logFile);
      break;
    }
    case dbg_step_into: {
      ea_t ip;
      thid_t tid;
      bool success1, success2;
      modinfo_t mod;
      success1 = get_ip_val(&ip);
      tid = get_current_thread();
      success2 = get_module_info(ip, &mod);
      if (success1 && success2)
      {
        insn_t insn;
        int insn_len;
        qstring insn_mnem;
        qstring insn_op0;
        op_t op;
        bool success3, success4, success5;
        insn_len = decode_insn(&insn, ip);
        if (insn_len != 0)
        {
          success3 = print_insn_mnem(&insn_mnem, ip);
          if (success3)
          {
            if (insn_mnem == "call")
            {
              op = insn.ops[0];
              if (op.type == o_mem) {
                ea_t api_call;
                modinfo_t api_mod;
                invalidate_dbgmem_contents(op.addr, sizeof(ea_t));
                api_call = (ea_t)get_dword(op.addr);
                success5 = get_module_info(api_call, &api_mod);
                if (success5)
                {
                  //Step over the known API function without going inside it
                  if (isKnownModule(api_mod.name)) {
                    
                    //you may need to log it
                    qfprintf(puser_data.logFile, "%p,%p,%s (callskip)\n", tid, ip, mod.name.c_str());
                    //then step over it
                    request_step_over();
                    return 0;
                  }

                }
              }
              else if (op.type == o_near)
              {
                ea_t near_call;
                //invalidate_dbgmem_contents(op.addr, sizeof(ea_t));
                //near_call = (ea_t)get_dword(op.addr);
                

                if (isNearCallWhiteListed(op.addr))
                {
                  request_step_over();
                  return 0;
                }
                else
                {
                  msg("call near %p\n", op.addr);
                  nearWhitelist.insert(op.addr);
                }

              }
              else if (op.type == o_reg)
              {
                msg("reg call\n");
                request_step_over();
                return 0;
              }
              else if (op.type == o_displ)
              {
                qstring reg_name;
                uint64 reg_val;
                ea_t displ_call_addr;
                ea_t displ_call;
                reg_val = 0;
                get_reg_name(&reg_name, op.reg, sizeof(ea_t));
                get_reg_val(reg_name.c_str(), &reg_val);
                displ_call_addr = reg_val + op.addr;

                invalidate_dbgmem_contents(displ_call_addr, sizeof(ea_t));
                displ_call = (ea_t)get_dword(displ_call_addr);
                
                if (isDisplCallWhiteListed(displ_call))
                {
                  request_step_over();
                  return 0;
                }
                else
                {
                  msg("displ call @ %p: %s => [%p] = %p\n", ip, reg_name.c_str(), displ_call_addr, displ_call);
                  displWhitelist.insert(displ_call);
                }

              }
              else
              {
                msg("other call\n");
                request_step_over();
                return 0;
              }
            }
          }
        }

        if (isKnownModule(mod.name)) {
          request_step_over();
        }
        else
        {
          //msg("Automated step into: %s\n", mod.name.c_str());
          //dbg_add_tev(tev_type_t::tev_bpt, tid, ip);
          //msg("step_into: %p\n", puser_data);
          //qfprintf(fp, "thread,ip,module\n");
          if (!isWhiteListed(ip))
          {
            qfprintf(puser_data.logFile, "%p,%p,%s\n", tid, ip, mod.name.c_str());
          }
          request_step_into();
        }
      }
      break;
    }
    case dbg_step_over: {
      ea_t ip;
      thid_t tid;
      bool success1, success2;
      modinfo_t mod;
      success1 = get_ip_val(&ip);
      tid = get_current_thread();
      success2 = get_module_info(ip, &mod);
      if (success1 && success2)
      {
        if (isKnownModule(mod.name)) {
          request_step_over();
        }
        else
        {
          //msg("Automated step into: %s\n", mod.name.c_str());
          //dbg_add_tev(tev_type_t::tev_bpt, tid, ip);
          //msg("step_over: %p\n", puser_data);
          //qfprintf(puser_data->logFile, "thread,ip,module\n");
          if (!isWhiteListed(ip))
          {
            qfprintf(puser_data.logFile, "%p,%p,%s\n", tid, ip, mod.name.c_str());
          }
          request_step_into();
        }
      }
      break;
    }
  }

  return 0;
}

//--------------------------------------------------------------------------
bool idaapi instruction_logger_plugin_t::run(size_t)
{
  if (ph.id != PLFM_386) {
    msg("Only the processor 'Intel 80x86' is supported.\n");
    return false;
  }

  loadWhiteList();

  bool hook_to_notification_retval = hook_to_notification_point(HT_DBG, hook_callback);
  if (hook_to_notification_retval)
  {
    msg("hook to notification was installed successfully.\n");
  }
  else {
    msg("hook to notification was not installed.\n");
  }

  return true;
}

//--------------------------------------------------------------------------
static plugmod_t* idaapi init()
{
  processor_t& ph = PH;
  return new instruction_logger_plugin_t(ph);
}

//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_FIX            // Load plugin when IDA starts and keep it in the memory until IDA stops.
  | PLUGIN_MULTI,       // The plugin can work with multiple idbs in parallel
  init,                 // initialize
  nullptr,
  nullptr,
  nullptr,              // long comment about the plugin
  nullptr,              // multiline help about the plugin
  "Instruction Logger 2 Plugin",       // the preferred short name of the plugin
  nullptr,              // the preferred hotkey to run the plugin
};
