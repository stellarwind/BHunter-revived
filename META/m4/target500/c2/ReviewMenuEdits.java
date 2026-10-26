import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.SourceType;
public class ReviewMenuEdits extends GhidraScript {public void run()throws Exception{
if(getFunctionAt(toAddr("0042d1b0"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042d1b0",toAddr("0042d1b0"),new AddressSet(toAddr("0042d1b0"),toAddr("0042d282")),SourceType.USER_DEFINED);disassemble(toAddr("0042d1b0"));
if(getFunctionAt(toAddr("0042d290"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042d290",toAddr("0042d290"),new AddressSet(toAddr("0042d290"),toAddr("0042d372")),SourceType.USER_DEFINED);disassemble(toAddr("0042d290"));
if(getFunctionAt(toAddr("0042e300"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042e300",toAddr("0042e300"),new AddressSet(toAddr("0042e300"),toAddr("0042e35d")),SourceType.USER_DEFINED);disassemble(toAddr("0042e300"));
if(getFunctionAt(toAddr("0042d380"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042d380",toAddr("0042d380"),new AddressSet(toAddr("0042d380"),toAddr("0042d518")),SourceType.USER_DEFINED);disassemble(toAddr("0042d380"));
if(getFunctionAt(toAddr("0042d550"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042d550",toAddr("0042d550"),new AddressSet(toAddr("0042d550"),toAddr("0042d5f2")),SourceType.USER_DEFINED);disassemble(toAddr("0042d550"));
if(getFunctionAt(toAddr("0042d620"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042d620",toAddr("0042d620"),new AddressSet(toAddr("0042d620"),toAddr("0042d6c2")),SourceType.USER_DEFINED);disassemble(toAddr("0042d620"));
if(getFunctionAt(toAddr("0042d6f0"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042d6f0",toAddr("0042d6f0"),new AddressSet(toAddr("0042d6f0"),toAddr("0042d8e7")),SourceType.USER_DEFINED);disassemble(toAddr("0042d6f0"));
if(getFunctionAt(toAddr("0042d920"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042d920",toAddr("0042d920"),new AddressSet(toAddr("0042d920"),toAddr("0042d98d")),SourceType.USER_DEFINED);disassemble(toAddr("0042d920"));
if(getFunctionAt(toAddr("0042d990"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042d990",toAddr("0042d990"),new AddressSet(toAddr("0042d990"),toAddr("0042da3a")),SourceType.USER_DEFINED);disassemble(toAddr("0042d990"));
if(getFunctionAt(toAddr("0042daa0"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042daa0",toAddr("0042daa0"),new AddressSet(toAddr("0042daa0"),toAddr("0042db42")),SourceType.USER_DEFINED);disassemble(toAddr("0042daa0"));
if(getFunctionAt(toAddr("0042da40"))==null)currentProgram.getFunctionManager().createFunction("FUN_0042da40",toAddr("0042da40"),new AddressSet(toAddr("0042da40"),toAddr("0042da97")),SourceType.USER_DEFINED);disassemble(toAddr("0042da40"));
}}
