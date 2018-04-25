EXEC = omp2hs

CFLAGS = -fopenmp -Wall -g3 -ggdb -O0 -std=c++11 -I/home/moon/local/include
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

OBJS = rewriter.o writeInFile.o

$(EXEC): ${OBJS}
	${CC} ${OBJS} ${CLANGLIBS} ${LDCONFIG}  -o ../bin/$(EXEC)

rewriter.o: rewriter.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c rewriter.cpp

writeInFile.o: writeInFile.cpp
	$(CC) $(CFLAGS) -c -o $@ $^ $(LDFLAGS)

clean:
	rm ../bin/$(EXEC) *.o 
