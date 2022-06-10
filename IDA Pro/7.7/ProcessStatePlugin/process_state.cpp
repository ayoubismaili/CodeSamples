#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <dbg.hpp>

//--------------------------------------------------------------------------
struct process_state_plugin_t : public plugmod_t
{
  processor_t& ph;
  process_state_plugin_t(processor_t& _ph) : ph(_ph) {}
  virtual bool idaapi run(size_t) override;
};

//--------------------------------------------------------------------------
bool idaapi process_state_plugin_t::run(size_t)
{
  if (ph.id != PLFM_386) {
    msg("Only the processor 'Intel 80x86' is supported.\n");
    return false;
  }

  int process_state = get_process_state();
  switch (process_state) {
    case DSTATE_NOTASK: {
      msg("no process is currently debugged.\n");
      break;
    }
    case DSTATE_SUSP: {
     msg("process is suspended and will not continue.\n");
     break;
    }
    case DSTATE_RUN: {
     msg("process is running.\n");
     break;
    }
    default: {
     msg("state is unknown.\n");
     break;
    }
  }
  return true;
}

//--------------------------------------------------------------------------
static plugmod_t *idaapi init()
{
  processor_t& ph = PH;
  return new process_state_plugin_t(ph);
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
  "Process State Plugin",       // the preferred short name of the plugin
  nullptr,              // the preferred hotkey to run the plugin
};
