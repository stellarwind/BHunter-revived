import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.SourceType;
public class DefineMoreWeaponOwners extends GhidraScript {
 void create(long a,long z)throws Exception{if(currentProgram.getFunctionManager().getFunctionAt(toAddr(a))==null)currentProgram.getFunctionManager().createFunction("FUN_"+Long.toHexString(a),toAddr(a),new AddressSet(toAddr(a),toAddr(z)),SourceType.USER_DEFINED);}
 public void run()throws Exception{create(0x4327a0L,0x432c50L);create(0x4331c0L,0x433819L);create(0x433850L,0x433af5L);create(0x433b30L,0x433db2L);println("Four reviewed weapon method bodies added to analysis only.");}
}
