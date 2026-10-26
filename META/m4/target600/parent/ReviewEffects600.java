import ghidra.app.script.GhidraScript;import ghidra.program.model.address.*;import ghidra.program.model.symbol.SourceType;public class ReviewEffects600 extends GhidraScript {public void run()throws Exception{
if(getFunctionAt(toAddr("00405ef0"))==null)currentProgram.getFunctionManager().createFunction("FUN_00405ef0",toAddr("00405ef0"),new AddressSet(toAddr("00405ef0"),toAddr("004062ca")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("00406300"))==null)currentProgram.getFunctionManager().createFunction("FUN_00406300",toAddr("00406300"),new AddressSet(toAddr("00406300"),toAddr("004067e0")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("00403870"))==null)currentProgram.getFunctionManager().createFunction("FUN_00403870",toAddr("00403870"),new AddressSet(toAddr("00403870"),toAddr("004044bf")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("00405270"))==null)currentProgram.getFunctionManager().createFunction("FUN_00405270",toAddr("00405270"),new AddressSet(toAddr("00405270"),toAddr("00405eef")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("004047c0"))==null)currentProgram.getFunctionManager().createFunction("FUN_004047c0",toAddr("004047c0"),new AddressSet(toAddr("004047c0"),toAddr("00405269")),SourceType.USER_DEFINED);
}}
