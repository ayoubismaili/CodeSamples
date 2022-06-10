#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <dbg.hpp>

//--------------------------------------------------------------------------
struct start_process_plugin_t : public plugmod_t
{
  processor_t& ph;
  start_process_plugin_t(processor_t& _ph) : ph(_ph) {}
  virtual bool idaapi run(size_t) override;
};

//--------------------------------------------------------------------------
bool idaapi start_process_plugin_t::run(size_t)
{
  if (ph.id != PLFM_386) {
    msg("Only the processor 'Intel 80x86' is supported.\n");
    return false;
  }

  int start_process_retval = start_process();

  switch (start_process_retval) {
    case -1: {
      msg("impossible to create the process.\n");
      break;
    }
    case 0: {
     msg("the starting of the process was cancelled by the user.\n");
     break;
    }
    case 1: {
     msg("the process was properly started.\n");
     break;
    }
    default: {
     msg("unknown error.\n");
     break;
    }
  }
  return true;
}

//--------------------------------------------------------------------------
static plugmod_t *idaapi init()
{
  processor_t& ph = PH;
  return new start_process_plugin_t(ph);
}

//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_UNL            // Unload the plugin immediately after calling 'run'
  | PLUGIN_MULTI,       // The plugin can work with multiple idbs in parallel
  init,                 // initialize
  nullptr,
  nullptr,
  nullptr,              // long comment about the plugin
  nullptr,              // multiline help about the plugin
  "Start Process Plugin",       // the preferred short name of the plugin
  nullptr,              // the preferred hotkey to run the plugin
};
