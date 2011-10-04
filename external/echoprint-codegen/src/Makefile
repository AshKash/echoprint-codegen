#OPTFLAGS=-g -O0
OPTFLAGS=-O3 -DBOOST_UBLAS_NDEBUG -DNDEBUG
CXXFLAGS=-Wall -fPIC $(OPTFLAGS)
CFLAGS=-Wall -fPIC $(OPTFLAGS)
#LDFLAGS=-lz $(OPTFLAGS)
SUFFIXES += .P

#We don't need to clean up when we're making these targets
NODEPS:=clean tags svn

#Find all the C++ files in the directory
SOURCES:=$(shell find . -name "*.cxx")
#These are the dependency files, which make will clean up after it creates them
DEPFILES:=$(patsubst %.cxx, .deps/%.P, $(SOURCES))
OBJFILES:=$(patsubst %.cxx, %.o, $(SOURCES))
PROG:=../lmcodegen
.DEFAULT_GOAL:=$(PROG)

#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
    #Chances are, these files don't exist.  GMake will create them and
    #clean up automatically afterwards
    -include $(DEPFILES)
endif

$(PROG): $(OBJFILES)
	$(CXX) $(LDFLAGS) -o $@ $(OBJFILES)

.deps:
	@mkdir -p .deps

#This is the rule for creating the dependency files
.deps/%.P: %.cxx .deps 
	$(CXX) $(CXXFLAGS) -MM -MT '$(patsubst %.cxx,%.o,$<)' $< > $@

#This rule does the compilation
%.o: %.cxx .deps/%.P
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean: 
	@rm -rf .deps
	@rm -f *.o
	@rm -f $(PROG)
