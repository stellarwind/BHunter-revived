// Read-only M1 export. No function creation, annotations, types or bytes changed.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class ReviewFrontier extends GhidraScript {
    String clean(Object value) { return String.valueOf(value).replace('\t',' ').replace('\n',' ').replace('\r',' '); }
    public void run() throws Exception {
        String[] args=getScriptArgs();
        Path out=Paths.get(args[0]); Files.createDirectories(out);
        FunctionManager fm=currentProgram.getFunctionManager();
        Listing listing=currentProgram.getListing();
        List<String> boundaries=new ArrayList<>();
        boundaries.add("address\tname\tranges\tsignature\tthunk\ttags");
        List<String> flow=new ArrayList<>(), pointers=new ArrayList<>();
        flow.add("function\tfrom\tflow_type\tcomputed\tto\ttarget_owner\tinstruction");
        pointers.add("target\tfrom\ttype\tsource_owner");
        FunctionIterator fi=fm.getFunctions(true);
        while(fi.hasNext()) {
            Function f=fi.next(); List<String> ranges=new ArrayList<>(), tags=new ArrayList<>();
            AddressRangeIterator ri=f.getBody().getAddressRanges();
            while(ri.hasNext()) { AddressRange r=ri.next(); ranges.add(r.getMinAddress()+"-"+r.getMaxAddress()); }
            for(FunctionTag t:f.getTags()) tags.add(t.getName());
            boundaries.add(f.getEntryPoint()+"\t"+clean(f.getName())+"\t"+String.join("|",ranges)+"\t"+clean(f.getSignature())+"\t"+f.isThunk()+"\t"+String.join("|",tags));
            ReferenceIterator incoming=currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint());
            while(incoming.hasNext()) {
                Reference ref=incoming.next();
                if(ref.getReferenceType().isData()) {
                    Function source=fm.getFunctionContaining(ref.getFromAddress());
                    pointers.add(f.getEntryPoint()+"\t"+ref.getFromAddress()+"\t"+ref.getReferenceType()+"\t"+(source==null?"":source.getEntryPoint()));
                }
            }
            InstructionIterator code=listing.getInstructions(f.getBody(),true);
            while(code.hasNext()) {
                Instruction ins=code.next();
                if(!ins.getFlowType().isCall() && !ins.getFlowType().isJump()) continue;
                Address[] targets=ins.getFlows();
                if(targets.length==0 && ins.getFlowType().isComputed())
                    flow.add(f.getEntryPoint()+"\t"+ins.getAddress()+"\t"+ins.getFlowType()+"\ttrue\t\t\t"+clean(ins));
                for(Address target:targets) {
                    Function owner=fm.getFunctionContaining(target);
                    if(ins.getFlowType().isCall() || !f.getBody().contains(target))
                        flow.add(f.getEntryPoint()+"\t"+ins.getAddress()+"\t"+ins.getFlowType()+"\t"+ins.getFlowType().isComputed()+"\t"+target+"\t"+(owner==null?"":owner.getEntryPoint())+"\t"+clean(ins));
                }
            }
        }
        Files.write(out.resolve("boundaries.tsv"),boundaries,StandardCharsets.UTF_8);
        Files.write(out.resolve("control-flow.tsv"),flow,StandardCharsets.UTF_8);
        Files.write(out.resolve("code-pointers.tsv"),pointers,StandardCharsets.UTF_8);
        List<String> tables=new ArrayList<>(); tables.add("slot\tvalue\tfunction_owner");
        long[][] spans={{0x491880L,0x49188cL},{0x492ddcL,0x492df0L},{0x48fa60L,0x48fb50L},{0x4f95f8L,0x4f961cL}};
        for(long[] span:spans) for(long va=span[0];va<span[1];va+=4) {
            int value=currentProgram.getMemory().getInt(toAddr(va));
            Function owner=fm.getFunctionContaining(toAddr(Integer.toUnsignedLong(value)));
            tables.add(toAddr(va)+"\t"+String.format("%08x",value)+"\t"+(owner==null?"":owner.getEntryPoint()));
        }
        Files.write(out.resolve("table-words.tsv"),tables,StandardCharsets.UTF_8);
        List<String> refs=new ArrayList<>();
        refs.add("function\tfrom\tto\ttype\ttarget_owner\ttarget_data");
        DecompInterface decompiler=new DecompInterface(); decompiler.openProgram(currentProgram);
        for(int n=1;n<args.length;n++) {
            monitor.checkCancelled();
            Function f=fm.getFunctionAt(toAddr(args[n]));
            if(f==null) { println("NO FUNCTION "+args[n]); continue; }
            List<String> asm=new ArrayList<>();
            asm.add("# "+f.getEntryPoint()+" "+f.getSignature()+" body="+f.getBody());
            ReferenceIterator incoming=currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint());
            while(incoming.hasNext()) asm.add("# IN "+incoming.next());
            InstructionIterator ii=listing.getInstructions(f.getBody(),true);
            while(ii.hasNext()) {
                Instruction ins=ii.next(); asm.add(ins.getAddress()+" "+ins);
                for(Reference ref:ins.getReferencesFrom()) {
                    Address target=ref.getToAddress(); Function owner=fm.getFunctionContaining(target);
                    Data data=listing.getDefinedDataContaining(target);
                    refs.add(f.getEntryPoint()+"\t"+ins.getAddress()+"\t"+target+"\t"+ref.getReferenceType()+"\t"+(owner==null?"":owner.getEntryPoint())+"\t"+(data==null?"":clean(data.getDataType()+" "+data.getValue())));
                }
            }
            Files.write(out.resolve(f.getEntryPoint()+".asm.txt"),asm,StandardCharsets.UTF_8);
            DecompileResults dr=decompiler.decompileFunction(f,60,monitor);
            if(!dr.decompileCompleted()) throw new Exception("Decompile failed: "+f.getEntryPoint()+" "+dr.getErrorMessage());
            Files.write(out.resolve(f.getEntryPoint()+".c.txt"),dr.getDecompiledFunction().getC().getBytes(StandardCharsets.UTF_8));
            println("Reviewed export "+f.getEntryPoint()+" "+f.getName());
        }
        decompiler.dispose();
        Files.write(out.resolve("references.tsv"),refs,StandardCharsets.UTF_8);
    }
}
