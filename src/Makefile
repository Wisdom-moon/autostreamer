EXEC = test

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

$(EXEC): test.cpp writeInFile.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

OBJS = rewriter.o writeInFile.o

omp2hs: ${OBJS}
	${CC} ${OBJS} ${CLANGLIBS} ${LDCONFIG}  -o ../bin/omp2hs

rewriter.o: rewriter.cpp
	${CC} ${CFLAGS} ${LCCONFIG} -c rewriter.cpp

writeInFile.o: writeInFile.cpp
	$(CC) $(CFLAGS) -c -o $@ $^ $(LDFLAGS)

clean:
	rm ../bin/omp2hs *.o $(EXEC)
