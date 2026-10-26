import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.SourceType;
public class ReviewMenuRuntime550 extends GhidraScript {public void run() throws Exception {
 if(getFunctionAt(toAddr("0042b2e0"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042b2e0",toAddr("0042b2e0"),new AddressSet(toAddr("0042b2e0"),toAddr("0042be6a")),SourceType.USER_DEFINED);
}}
