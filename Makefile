TARGET = convert

# Compile Setting
CC      = g++
CXX      = g++
CFLAGS  =  -I/usr/local/root/include 

#Suffix Setting
CSuffix = .c
CXXSuffix = .cxx
HeaderSuffix = .h
ObjectSuffix = .o
ExecuteSuffix =

# PMTTester Setting
#PMTTesterRoot =  $(shell echo $$PMTROOT)
#CFLAGS += -I$(PMTTesterRoot)/include/

# ROOT
ROOTCFLAGS = $(shell root-config --cflags)
ROOTLIBS = $(shell root-config --libs)

EXE = $(TARGET)$(ExecuteSuffix)

# A: B
# $@ = A, $^ = B

all: $(EXE)

$(EXE): $(TARGET)$(ObjectSuffix) 
	$(CXX) $(ROOTLIBS) -o $@ $^

$(TARGET)$(ObjectSuffix): $(TARGET)$(CXXSuffix)  
	$(CXX) -c $(CFLAGS) $(ROOTCFLAGS) -o $@ $^

# clean Function
clean:
	rm -f *~
	rm -f *.o
	rm -f $(EXE)
