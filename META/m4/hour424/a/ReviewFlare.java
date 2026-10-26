import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
public class ReviewFlare extends GhidraScript {
 public void run()throws Exception{
 String[]v={"004035e0","00478540","00477ae0"};int[]n={82,167,570};Path dir=Paths.get("D:/Decomp/Bhunter/META/m4/hour424/a/original");Files.createDirectories(dir);
 for(int i=0;i<v.length;i++){Address p=toAddr(v[i]);if(getFunctionAt(p)==null&&getFunctionContaining(p)==null)currentProgram.getFunctionManager().createFunction("FUN_"+v[i],p,new AddressSet(p,p.add(n[i]-1)),ghidra.program.model.symbol.SourceType.USER_DEFINED);disassemble(p);
 Function f=getFunctionAt(p);StringBuilder out=new StringBuilder("[");boolean first=true;InstructionIterator it=currentProgram.getListing().getInstructions(f.getBody(),true);
 while(it.hasNext()){Instruction ins=it.next();StringBuilder hex=new StringBuilder();for(byte b:ins.getBytes())hex.append(String.format("%02x",b&255));if(!first)out.append(',');first=false;out.append("{\"address\":\"").append(ins.getAddress()).append("\",\"bytes\":\"").append(hex).append("\",\"assembly\":\"").append(ins.toString().replace("\\","\\\\").replace("\"","\\\"")).append("\"}");}
 Files.write(dir.resolve(v[i]+".disasm.json"),out.append("]\n").toString().getBytes(StandardCharsets.UTF_8));}
 }
}
