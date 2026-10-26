import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
public class ReviewPulse extends GhidraScript{
 public void run()throws Exception{String[]a={"00402e50","00403120","00402cc0","00402d20","004027f0"};int[]n={706,451,83,242,58};
 for(int i=0;i<a.length;i++){Address p=toAddr(a[i]);if(getFunctionAt(p)==null&&getFunctionContaining(p)==null)currentProgram.getFunctionManager().createFunction("FUN_"+a[i],p,new AddressSet(p,p.add(n[i]-1)),ghidra.program.model.symbol.SourceType.USER_DEFINED);disassemble(p);}}
}
