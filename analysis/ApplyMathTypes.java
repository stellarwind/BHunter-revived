import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;

public class ApplyMathTypes extends GhidraScript {
    public void run() throws Exception {
        DataTypeManager dtm=currentProgram.getDataTypeManager();
        CategoryPath category=new CategoryPath("/BH/Math");
        StructureDataType vector=new StructureDataType(category,"BHVec3",0);
        vector.add(FloatDataType.dataType,"x",null);
        vector.add(FloatDataType.dataType,"y",null);
        vector.add(FloatDataType.dataType,"z",null);
        DataType vec=dtm.addDataType(vector,DataTypeConflictHandler.REPLACE_HANDLER);
        StructureDataType matrix=new StructureDataType(category,"BHMat34",0);
        matrix.add(new ArrayDataType(new ArrayDataType(FloatDataType.dataType,4,4),3,16),"m","3 rows of 4 floats; translation in column 3");
        DataType mat=dtm.addDataType(matrix,DataTypeConflictHandler.REPLACE_HANDLER);
        DataType vp=new PointerDataType(vec),mp=new PointerDataType(mat),fl=FloatDataType.dataType;
        long[] addresses={0x401000,0x4011a0,0x4011f0,0x401270,0x4012e0,0x401360,0x4013d0,0x401460,0x401490,0x401a70};
        String[] names={"bh_mat34_multiply","bh_vec3_cross","bh_vec3_normalize","bh_vec3_normalize_to","bh_mat34_transform_point","bh_mat34_transform_direction","bh_mat34_inverse_transform_point","bh_mat34_translate_world","bh_mat34_translate_local","bh_mat34_inverse_rigid"};
        DataType[][] types={{mp,mp,mp},{vp,vp,vp},{vp},{vp,vp},{mp,vp,vp},{mp,vp,vp},{mp,vp,vp},{fl,fl,fl,mp},{fl,fl,fl,mp},{mp,mp}};
        String[][] parameters={{"a","b","out"},{"a","b","out"},{"v"},{"v","out"},{"m","v","out"},{"m","v","out"},{"m","v","out"},{"x","y","z","m"},{"x","y","z","m"},{"m","out"}};
        for(int i=0;i<addresses.length;i++) {
            Function f=currentProgram.getFunctionManager().getFunctionAt(toAddr(addresses[i]));
            if(f==null) throw new Exception("Missing function "+names[i]);
            f.setName(names[i],SourceType.USER_DEFINED);
            f.setCallingConvention("__cdecl");
            f.setReturnType(i==2||i==3 ? IntegerDataType.dataType : VoidDataType.dataType,SourceType.USER_DEFINED);
            Parameter[] p=new Parameter[types[i].length];
            for(int j=0;j<p.length;j++) p[j]=new ParameterImpl(parameters[i][j],types[i][j],currentProgram);
            f.replaceParameters(Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,true,SourceType.USER_DEFINED,p);
            f.addTag("recovered-math-finite-tested");
            setPlateComment(f.getEntryPoint(),"Recovered layout and behavior tested against original isolated machine code. See src/bh_math.c and analysis/math/RECOVERY.md. Not byte-matched; aliasing and floating-point limits documented there.");
        }
        println("Applied 12-byte BHVec3, 48-byte BHMat34, and 10 cdecl signatures.");
    }
}
