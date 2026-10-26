import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
public class ReviewTarget extends GhidraScript{
 public void run()throws Exception{Address p=toAddr("0043cd00");if(getFunctionAt(p)==null)currentProgram.getFunctionManager().createFunction("FUN_0043cd00",p,new AddressSet(p,toAddr("0043d27b")),ghidra.program.model.symbol.SourceType.USER_DEFINED);disassemble(p);println(getFunctionAt(p).toString());}
}
