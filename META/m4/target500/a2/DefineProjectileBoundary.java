import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.symbol.SourceType;
public class DefineProjectileBoundary extends GhidraScript {
 public void run() throws Exception {
  // Private analysis project metadata only. Immutable executable never written.
  long va=0x43f630L;int size=2549;
  if(currentProgram.getFunctionManager().getFunctionAt(toAddr(va))==null)
   currentProgram.getFunctionManager().createFunction("FUN_0043f630",toAddr(va),new AddressSet(toAddr(va),toAddr(va+size-1)),SourceType.USER_DEFINED);
 }
}
