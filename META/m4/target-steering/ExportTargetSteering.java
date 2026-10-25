import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.io.*;
public class ExportTargetSteering extends GhidraScript {
 public void run() throws Exception {
  File out=new File(getScriptArgs()[0]);out.mkdirs();
  for(String a:new String[]{"004340f0","004344f0","00410880","004084a0","00434650","00434810","004355c0","00434f40","004343c0","00435790","00435720"}) {
   Function f=getFunctionAt(toAddr(a));AddressSetView body;
   if(f!=null)body=f.getBody();else if(a.equals("00435790"))body=new AddressSet(toAddr(a),toAddr(a).add(57));else if(a.equals("00435720"))body=new AddressSet(toAddr(a),toAddr(a).add(101));else throw new IllegalStateException(a);
   FileWriter w=new FileWriter(new File(out,a+".tsv"));w.write("address\tbytes\tinstruction\n");
   InstructionIterator it=currentProgram.getListing().getInstructions(body,true);
   while(it.hasNext()){Instruction i=it.next();StringBuilder h=new StringBuilder();for(byte b:i.getBytes())h.append(String.format("%02x",b&255));w.write(i.getAddress()+"\t"+h+"\t"+i+"\n");}
   w.close();println(a+" "+body+" function="+(f!=null));
  }
 }
}
