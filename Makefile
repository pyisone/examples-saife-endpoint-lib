CXX=g++


SOURCES = src/saife_msg_demo.cc
ifndef SAIFE_SDK_HOME 
$(error SAIFE_SDK_HOME not set)
endif

SAIFE_INCLUDES := -I$(SAIFE_SDK_HOME)/include
SAIFE_ADAPTER_LIBS := -L$(SAIFE_SDK_HOME)/lib/saife -lsaife -lCecCryptoEngine -lroxml 

CXXFLAGS= $(SAIFE_INCLUDES) -std=c++0x
LIBS = $(SAIFE_ADAPTER_LIBS)


sender: 
	$(CXX) $(CXXFLAGS) -o saife_msg_demo $(SOURCES) $(LIBS)

.PHONY: clean

clean:
	rm saife_msg_demo
