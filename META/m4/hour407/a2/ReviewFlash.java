import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
public class ReviewFlash extends GhidraScript {
 public void run()throws Exception{
 String[]v={"004035e0","00403640","00478540"};int[]n={82,45,167};
 for(int i=0;i<v.length;i++){Address p=toAddr(v[i]);if(getFunctionAt(p)==null&&getFunctionContaining(p)==null)currentProgram.getFunctionManager().createFunction("FUN_"+v[i],p,new AddressSet(p,p.add(n[i]-1)),ghidra.program.model.symbol.SourceType.USER_DEFINED);disassemble(p);}
 }
}
