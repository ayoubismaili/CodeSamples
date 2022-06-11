#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <dbg.hpp>

//--------------------------------------------------------------------------
struct step_over_plugin_t : public plugmod_t
{
  processor_t& ph;
  step_over_plugin_t(processor_t& _ph) : ph(_ph) {}
  virtual bool idaapi run(size_t) override;
};

//--------------------------------------------------------------------------
bool idaapi step_over_plugin_t::run(size_t)
{
  if (ph.id != PLFM_386) {
    msg("Only the processor 'Intel 80x86' is supported.\n");
    return false;
  }

  int step_over_retval = step_over();

  if (step_over_retval) {
    msg("step over was successful.\n");
  }
  else {
    msg("step over was failed.\n");
  }
  return true;
}

//--------------------------------------------------------------------------
static plugmod_t *idaapi init()
{
  processor_t& ph = PH;
  return new step_over_plugin_t(ph);
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
  "Step Over Plugin",       // the preferred short name of the plugin
  nullptr,              // the preferred hotkey to run the plugin
};
