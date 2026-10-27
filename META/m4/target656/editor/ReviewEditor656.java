import ghidra.app.script.GhidraScript;import ghidra.program.model.address.*;import ghidra.program.model.symbol.SourceType;public class ReviewEditor656 extends GhidraScript {public void run()throws Exception{
if(getFunctionAt(toAddr("0040c660"))==null)currentProgram.getFunctionManager().createFunction("FUN_0040c660",toAddr("0040c660"),new AddressSet(toAddr("0040c660"),toAddr("0040c9ac")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0040c9b0"))==null)currentProgram.getFunctionManager().createFunction("FUN_0040c9b0",toAddr("0040c9b0"),new AddressSet(toAddr("0040c9b0"),toAddr("0040cff9")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0040d140"))==null)currentProgram.getFunctionManager().createFunction("FUN_0040d140",toAddr("0040d140"),new AddressSet(toAddr("0040d140"),toAddr("0040dff8")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0040e070"))==null)currentProgram.getFunctionManager().createFunction("FUN_0040e070",toAddr("0040e070"),new AddressSet(toAddr("0040e070"),toAddr("0040e1f8")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0040e200"))==null)currentProgram.getFunctionManager().createFunction("FUN_0040e200",toAddr("0040e200"),new AddressSet(toAddr("0040e200"),toAddr("0040e2f7")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0040e300"))==null)currentProgram.getFunctionManager().createFunction("FUN_0040e300",toAddr("0040e300"),new AddressSet(toAddr("0040e300"),toAddr("0040e414")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0040e420"))==null)currentProgram.getFunctionManager().createFunction("FUN_0040e420",toAddr("0040e420"),new AddressSet(toAddr("0040e420"),toAddr("0040fe74")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("00410080"))==null)currentProgram.getFunctionManager().createFunction("FUN_00410080",toAddr("00410080"),new AddressSet(toAddr("00410080"),toAddr("004100ab")),SourceType.USER_DEFINED);
}}
