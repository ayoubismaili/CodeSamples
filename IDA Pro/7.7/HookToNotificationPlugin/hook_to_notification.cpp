#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <dbg.hpp>

//--------------------------------------------------------------------------
struct hook_to_notification_plugin_t : public plugmod_t
{
  processor_t& ph;
  hook_to_notification_plugin_t(processor_t& _ph) : ph(_ph) {}
  virtual bool idaapi run(size_t) override;
};

//--------------------------------------------------------------------------
ssize_t idaapi hook_callback(void* user_data, int notification_code, va_list va) {

  switch (notification_code) {
    case dbg_process_start: {
      msg("hook notification: Process Started\n");
      break;
    }
    case dbg_process_exit: {
      msg("hook notification: Process Exited\n");
      break;
    }
    case dbg_step_into: {
      msg("hook notification: Step Into Occurred\n");
      break;
    }
  }

  return 0;
}

//--------------------------------------------------------------------------
bool idaapi hook_to_notification_plugin_t::run(size_t)
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
  return new hook_to_notification_plugin_t(ph);
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
  "Hook to Notification Plugin",       // the preferred short name of the plugin
  nullptr,              // the preferred hotkey to run the plugin
};
