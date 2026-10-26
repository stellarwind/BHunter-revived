import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.SourceType;
public class DefineWeaponOwners extends GhidraScript {
 void create(long a,long z)throws Exception{if(currentProgram.getFunctionManager().getFunctionAt(toAddr(a))==null)currentProgram.getFunctionManager().createFunction("FUN_"+Long.toHexString(a),toAddr(a),new AddressSet(toAddr(a),toAddr(z)),SourceType.USER_DEFINED);}
 public void run()throws Exception{create(0x4305f0L,0x4309a7L);create(0x430a30L,0x430d7fL);create(0x430d80L,0x431042L);create(0x431810L,0x431b37L);create(0x431db0L,0x431ef9L);create(0x431f00L,0x43204bL);println("Six reviewed ordinary weapon method boundaries added to analysis; reference bytes unchanged.");}
}
