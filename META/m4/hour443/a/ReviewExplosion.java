import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
public class ReviewExplosion extends GhidraScript{
 public void run()throws Exception{String[]a={"00402e50","00402af0","004032f0","00402710"};int[]n={706,455,749,217};Path dir=Paths.get("D:/Decomp/Bhunter/META/m4/hour443/a/original");Files.createDirectories(dir);
 for(int i=0;i<a.length;i++){Address p=toAddr(a[i]);if(getFunctionAt(p)==null&&getFunctionContaining(p)==null)currentProgram.getFunctionManager().createFunction("FUN_"+a[i],p,new AddressSet(p,p.add(n[i]-1)),ghidra.program.model.symbol.SourceType.USER_DEFINED);disassemble(p);Function f=getFunctionAt(p);
 StringBuilder out=new StringBuilder("[");boolean first=true;InstructionIterator it=currentProgram.getListing().getInstructions(f.getBody(),true);
 while(it.hasNext()){Instruction ins=it.next();StringBuilder hex=new StringBuilder();for(byte b:ins.getBytes())hex.append(String.format("%02x",b&255));if(!first)out.append(',');first=false;out.append("{\"address\":\"").append(ins.getAddress()).append("\",\"bytes\":\"").append(hex).append("\",\"assembly\":\"").append(ins.toString().replace("\\","\\\\").replace("\"","\\\"")).append("\"}");}
 Files.write(dir.resolve(a[i]+".disasm.json"),out.append("]\n").toString().getBytes(StandardCharsets.UTF_8));}}
}
