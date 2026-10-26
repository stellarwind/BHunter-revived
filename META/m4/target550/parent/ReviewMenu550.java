import ghidra.app.script.GhidraScript;import ghidra.program.model.address.*;import ghidra.program.model.symbol.SourceType;public class ReviewMenu550 extends GhidraScript{public void run()throws Exception{
if(getFunctionAt(toAddr("0042cb40"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042cb40",toAddr("0042cb40"),new AddressSet(toAddr("0042cb40"),toAddr("0042cba1")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042cec0"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042cec0",toAddr("0042cec0"),new AddressSet(toAddr("0042cec0"),toAddr("0042cef3")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042cf00"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042cf00",toAddr("0042cf00"),new AddressSet(toAddr("0042cf00"),toAddr("0042cfa3")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042d070"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042d070",toAddr("0042d070"),new AddressSet(toAddr("0042d070"),toAddr("0042d0c1")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042d0d0"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042d0d0",toAddr("0042d0d0"),new AddressSet(toAddr("0042d0d0"),toAddr("0042d1a8")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042db60"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042db60",toAddr("0042db60"),new AddressSet(toAddr("0042db60"),toAddr("0042de5e")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042df90"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042df90",toAddr("0042df90"),new AddressSet(toAddr("0042df90"),toAddr("0042e14f")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042e150"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042e150",toAddr("0042e150"),new AddressSet(toAddr("0042e150"),toAddr("0042e2f1")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042e9e0"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042e9e0",toAddr("0042e9e0"),new AddressSet(toAddr("0042e9e0"),toAddr("0042ea61")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042eb30"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042eb30",toAddr("0042eb30"),new AddressSet(toAddr("0042eb30"),toAddr("0042ee39")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042eea0"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042eea0",toAddr("0042eea0"),new AddressSet(toAddr("0042eea0"),toAddr("0042f0b1")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042f280"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042f280",toAddr("0042f280"),new AddressSet(toAddr("0042f280"),toAddr("0042f2a8")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042f2b0"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042f2b0",toAddr("0042f2b0"),new AddressSet(toAddr("0042f2b0"),toAddr("0042f346")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042f360"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042f360",toAddr("0042f360"),new AddressSet(toAddr("0042f360"),toAddr("0042f388")),SourceType.USER_DEFINED);
if(getFunctionAt(toAddr("0042f390"))==null) currentProgram.getFunctionManager().createFunction("FUN_0042f390",toAddr("0042f390"),new AddressSet(toAddr("0042f390"),toAddr("0042f3f1")),SourceType.USER_DEFINED);
}}
