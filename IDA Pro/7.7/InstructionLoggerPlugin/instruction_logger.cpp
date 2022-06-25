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

qstring strToLower(qstring str) {
  qstring strOut = str;
  std::transform(strOut.begin(), strOut.end(), strOut.begin(),
    [](unsigned char c) { return std::tolower(c); });
  return strOut;
}

bool isKnownModule(qstring mod_name) {
  mod_name = strToLower(mod_name);
  return (mod_name == "ntdll.dll"
    || mod_name == "c:\\windows\\system32\\kernelbase.dll"
    || mod_name == "c:\\windows\\system32\\msvcrt.dll"
    || mod_name == "c:\\windows\\system32\\winbrand.dll");
}

//--------------------------------------------------------------------------
ssize_t idaapi hook_callback(void* user_data, int notification_code, va_list va) {

  switch (notification_code) {
    case dbg_process_start: {
      msg("hook notification: Process Started\n");
      request_clear_trace();
      break;
    }
    case dbg_process_exit: {
      msg("hook notification: Process Exited\n");
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
        if (isKnownModule(mod.name)) {
          request_step_over();
        }
        else 
        {
          msg("Automated step into: %s\n", mod.name.c_str());
          dbg_add_tev(tev_type_t::tev_bpt, tid, ip);
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
          msg("Automated step into: %s\n", mod.name.c_str());
          dbg_add_tev(tev_type_t::tev_bpt, tid, ip);
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
  "Instruction Logger Plugin",       // the preferred short name of the plugin
  nullptr,              // the preferred hotkey to run the plugin
};
