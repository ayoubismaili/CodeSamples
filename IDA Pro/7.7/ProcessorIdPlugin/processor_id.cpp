#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>

//--------------------------------------------------------------------------
struct processor_id_plugin_t : public plugmod_t
{
  processor_t& ph;
  processor_id_plugin_t(processor_t& _ph) : ph(_ph) {}
  virtual bool idaapi run(size_t) override;
};

//--------------------------------------------------------------------------
bool idaapi processor_id_plugin_t::run(size_t)
{
  if (ph.id == PLFM_386) {
    msg("PLFM_386: Intel 80x86\n");
  } else if (ph.id == PLFM_Z80) {
   msg("PLFM_Z80: 8085, Z80\n");
  }
  else {
   msg("Unknown Processor ID\n");
  }
  return true;
}

//--------------------------------------------------------------------------
static plugmod_t *idaapi init()
{
  processor_t& ph = PH;
  return new processor_id_plugin_t(ph);
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
  "Processor ID Plugin",       // the preferred short name of the plugin
  nullptr,              // the preferred hotkey to run the plugin
};
