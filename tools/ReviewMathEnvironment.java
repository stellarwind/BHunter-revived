// Read-only survey of x87 environment instructions and verified CRT controls.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class ReviewMathEnvironment extends GhidraScript {
    public void run() throws Exception {
        Path out=Paths.get(getScriptArgs()[0]);Files.createDirectories(out);
        FunctionManager fm=currentProgram.getFunctionManager();
        List<String> rows=new ArrayList<>();rows.add("address\tfunction\tname\tinstruction");
        InstructionIterator instructions=currentProgram.getListing().getInstructions(true);
        Set<String> mnemonics=new HashSet<>(Arrays.asList("FLDCW","FNSTCW","FSTCW","FINIT","FNINIT","FRSTOR","FLDENV","FXRSTOR","LDMXCSR"));
        while(instructions.hasNext()) {
            Instruction ins=instructions.next();
            if(!mnemonics.contains(ins.getMnemonicString().toUpperCase())) continue;
            Function owner=fm.getFunctionContaining(ins.getAddress());
            rows.add(ins.getAddress()+"\t"+(owner==null?"":owner.getEntryPoint())+"\t"+(owner==null?"":owner.getName())+"\t"+ins);
        }
        Files.write(out.resolve("environment-instructions.tsv"),rows,StandardCharsets.UTF_8);
        List<String> refs=new ArrayList<>();refs.add("target\tname\tfrom\towner\ttype");
        FunctionIterator functions=fm.getFunctions(true);
        while(functions.hasNext()) {
            Function f=functions.next();String name=f.getName();
            if(!(name.contains("control") || name.contains("ctrlfp") || name.contains("fpreset") || name.contains("setdefaultprecision") || name.equals("__fpmath"))) continue;
            ReferenceIterator incoming=currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint());
            while(incoming.hasNext()) {
                Reference r=incoming.next();Function owner=fm.getFunctionContaining(r.getFromAddress());
                refs.add(f.getEntryPoint()+"\t"+name+"\t"+r.getFromAddress()+"\t"+(owner==null?"":owner.getEntryPoint())+"\t"+r.getReferenceType());
            }
        }
        Files.write(out.resolve("control-references.tsv"),refs,StandardCharsets.UTF_8);
        println("x87 environment instructions="+(rows.size()-1)+"; control references="+(refs.size()-1));
    }
}
