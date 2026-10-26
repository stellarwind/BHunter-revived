import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.SourceType;
public class ReviewQueries extends GhidraScript { public void run() throws Exception {String[] a={"0040fe80","00410020","0043c220"},e={"0040ffeb","00410079","0043c3bc"};for(int i=0;i<a.length;i++){if(getFunctionAt(toAddr(a[i]))==null)currentProgram.getFunctionManager().createFunction("FUN_"+a[i],toAddr(a[i]),new AddressSet(toAddr(a[i]),toAddr(e[i])),SourceType.USER_DEFINED);disassemble(toAddr(a[i]));}String[] tables={"0040ffec","0043c3c0"};int[] counts={5,6};for(int i=0;i<2;i++)for(int j=0;j<counts[i];j++){Address p=toAddr(currentProgram.getMemory().getInt(toAddr(tables[i]).add(j*4))&0xffffffffL);disassemble(p);println(p.toString());}}}
