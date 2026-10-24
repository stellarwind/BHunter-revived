import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
public class SoftwareReview extends GhidraScript {
 public void run() throws Exception {
  Path out=Paths.get(getScriptArgs()[0]);Files.createDirectories(out);
  Function f=getFunctionAt(toAddr("0046e600"));
  DecompInterface d=new DecompInterface();d.openProgram(currentProgram);
  DecompileResults r=d.decompileFunction(f,30,monitor);
  Files.writeString(out.resolve("0046e600.c.txt"),r.getDecompiledFunction().getC().replace("\r\n","\n"),StandardCharsets.UTF_8);
  List<String> lines=new ArrayList<>();lines.add("address\tbytes\tinstruction");
  InstructionIterator instructions=currentProgram.getListing().getInstructions(f.getBody(),true);
  while(instructions.hasNext()){
   Instruction ins=instructions.next();StringBuilder bytes=new StringBuilder();for(byte v:ins.getBytes())bytes.append(String.format("%02x",v&255));
   lines.add(ins.getAddress()+"\t"+bytes+"\t"+ins.toString());
  }
  Files.writeString(out.resolve("0046e600.tsv"),String.join("\n",lines)+"\n",StandardCharsets.UTF_8);d.dispose();
 }
}
