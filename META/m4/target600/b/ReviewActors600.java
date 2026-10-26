import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
public class ReviewActors600 extends GhidraScript {public void run() throws Exception {String[] a={"004357d0"};int[] n={1709};for(int i=0;i<a.length;i++){Address p=toAddr(a[i]);if(getFunctionAt(p)==null&&getFunctionContaining(p)==null)currentProgram.getFunctionManager().createFunction("ActorReview600_"+a[i],p,new AddressSet(p,p.add(n[i]-1)),ghidra.program.model.symbol.SourceType.USER_DEFINED);println(a[i]+" "+getFunctionContaining(p));}}}
