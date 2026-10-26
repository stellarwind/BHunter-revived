import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.SourceType;
public class ReviewMenuActions extends GhidraScript {public void run()throws Exception{
if(getFunctionAt(toAddr("0042ca10"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042ca10",toAddr("0042ca10"),new AddressSet(toAddr("0042ca10"),toAddr("0042ca23")),SourceType.USER_DEFINED);disassemble(toAddr("0042ca10"));
if(getFunctionAt(toAddr("0042ca30"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042ca30",toAddr("0042ca30"),new AddressSet(toAddr("0042ca30"),toAddr("0042ca90")),SourceType.USER_DEFINED);disassemble(toAddr("0042ca30"));
if(getFunctionAt(toAddr("0042e410"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042e410",toAddr("0042e410"),new AddressSet(toAddr("0042e410"),toAddr("0042e51d")),SourceType.USER_DEFINED);disassemble(toAddr("0042e410"));
if(getFunctionAt(toAddr("0042e640"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042e640",toAddr("0042e640"),new AddressSet(toAddr("0042e640"),toAddr("0042e681")),SourceType.USER_DEFINED);disassemble(toAddr("0042e640"));
if(getFunctionAt(toAddr("0042e790"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042e790",toAddr("0042e790"),new AddressSet(toAddr("0042e790"),toAddr("0042e7d1")),SourceType.USER_DEFINED);disassemble(toAddr("0042e790"));
if(getFunctionAt(toAddr("0042e8f0"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042e8f0",toAddr("0042e8f0"),new AddressSet(toAddr("0042e8f0"),toAddr("0042e9cb")),SourceType.USER_DEFINED);disassemble(toAddr("0042e8f0"));
}}