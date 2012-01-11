SRCS := frandre.cpp kaguya.cpp kanako.cpp mainwnd.cpp pbgmgr.cpp \
        thcrypter.cpp unlzss.cpp yuyuko.cpp vivit.cpp hinanawi.cpp \
        suica.cpp mima.cpp yumemi.cpp unerle.cpp pathext.cpp
RESRCS := pbgmgr.rc

OBJS := $(SRCS:%.cpp=%.o) $(RESRCS:%.rc=%.res)
LIBS := -lcomctl32 -lcomdlg32 -lshlwapi -lole32 -limagehlp -lboost_filesystem-gcc-mt-s-1_33_1

CXX := g++
RM := rm
DRES := windres

CXXFLAGS := -O3 -Os -s -mno-cygwin -Wno-deprecated -D_WIN32_IE=0x600
LDFLAGS  := -O3 -Os -s -mno-cygwin
# CXXFLAGS := -g -mno-cygwin -Wno-deprecated -D_WIN32_IE=0x600
# LDFLAGS  := -g -mno-cygwin -Wl,-Map,brightmoon.map

all: brightmoon.exe

brightmoon.exe: ${OBJS}
	${CXX} -o $@ ${OBJS} -mwindows ${LDFLAGS} ${LIBS}

%.res: %.rc
	${DRES} -O coff $< $@

%.o: %.cpp
	${CXX} -c -o $@ $< ${CXXFLAGS}

dep:
	${CXX} -MM -MG ${SRCS}   >  Makefile.dep
	${CXX} -MM -MG ${RESRCS} >> Makefile.dep

clean:
	${RM} ${OBJS}

-include Makefile.dep
