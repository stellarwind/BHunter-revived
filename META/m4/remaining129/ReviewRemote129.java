import ghidra.app.script.GhidraScript;import ghidra.program.model.address.*;import ghidra.program.model.symbol.SourceType;public class ReviewRemote129 extends GhidraScript {public void run()throws Exception{
if(getFunctionAt(toAddr("00407930"))==null)currentProgram.getFunctionManager().createFunction("FUN_00407930",toAddr("00407930"),new AddressSet(toAddr("00407930"),toAddr("00407a12")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("00410080"))==null)currentProgram.getFunctionManager().createFunction("FUN_00410080",toAddr("00410080"),new AddressSet(toAddr("00410080"),toAddr("004100ab")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("00423c80"))==null)currentProgram.getFunctionManager().createFunction("FUN_00423c80",toAddr("00423c80"),new AddressSet(toAddr("00423c80"),toAddr("00423e61")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("00423e70"))==null)currentProgram.getFunctionManager().createFunction("FUN_00423e70",toAddr("00423e70"),new AddressSet(toAddr("00423e70"),toAddr("00424128")),SourceType.USER_DEFINED);
}}
