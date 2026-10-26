import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
public class ReviewTarget extends GhidraScript {
 public void run() throws Exception {
  Address p=toAddr("0043c3f0");
  if(getFunctionAt(p)==null&&getFunctionContaining(p)==null)currentProgram.getFunctionManager().createFunction("FUN_0043c3f0",p,new AddressSet(p,p.add(1214)),ghidra.program.model.symbol.SourceType.USER_DEFINED);
  disassemble(p);
 }
}
