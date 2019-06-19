EXEC = omp2hs

CFLAGS = -fopenmp -Wall -g3 -ggdb -O0 -std=c++11 -I/home/moon/local/include -I../include
LDFLAGS =
LDFLAGS += -L/home/zp/local/lib64/ -lltdl -lm -lgomp

LLCLANG = clang++ -stdlib=libc++
LDCONFIG = `llvm-config --ldflags --system-libs --libs`
LCCONFIG =  `llvm-config --cxxflags`
CLANGLIBS := \
        -Wl,--start-group \
        -lclangAST \
        -lclangASTMatchers \
        -lclangAnalysis \
        -lclangBasic \
        -lclangDriver \
        -lclangEdit \
        -lclangFrontend \
        -lclangFrontendTool \
        -lclangLex \
        -lclangParse \
        -lclangSema \
        -lclangEdit \
        -lclangRewrite \
        -lclangRewriteFrontend \
        -lclangStaticAnalyzerFrontend \
        -lclangStaticAnalyzerCheckers \
        -lclangStaticAnalyzerCore \
        -lclangSerialization \
        -lclangToolingCore \
        -lclangTooling \
        -lclangFormat \
        -Wl,--end-group


CC = g++
RM = rm -f

.PHONY: clean 

all: $(EXEC)

OBJS = main.o CodeGen.o FunctionInfo.o KernelExtractor.o LoopInfo.o PassManager.o ScopeIR.o ScopeIRGen.o Util.o

$(EXEC): ${OBJS}
	${CC} ${OBJS} ${CLANGLIBS} ${LDCONFIG}  -o ../bin/$(EXEC)

main.o: main.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^

ScopeIRGen.o: ScopeIRGen.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^
FunctionInfo.o: FunctionInfo.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^
KernelExtractor.o: KernelExtractor.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^
LoopInfo.o: LoopInfo.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^
PassManager.o: PassManager.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^
ScopeIR.o: ScopeIR.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^
Util.o: Util.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c -o $@ $^

CodeGen.o: CodeGen.cpp
	$(CC) $(CFLAGS) -c -o $@ $^ $(LDFLAGS)

clean:
	rm ../bin/$(EXEC) *.o 
