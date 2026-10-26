import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.symbol.SourceType;
public class DefineBoundaries extends GhidraScript {
 public void run() throws Exception {
  long[] starts={0x43e620L,0x43e710L,0x43e840L,0x440050L,0x441ce0L,0x441d80L};
  int[] sizes={231,293,589,1973,150,1454};
  for(int i=0;i<starts.length;i++) {
   if(getFunctionAt(toAddr(starts[i]))==null)currentProgram.getFunctionManager().createFunction("FUN_"+Long.toHexString(starts[i]),toAddr(starts[i]),new AddressSet(toAddr(starts[i]),toAddr(starts[i]+sizes[i]-1)),SourceType.USER_DEFINED);
   disassemble(toAddr(starts[i]));
  }
 }
}
