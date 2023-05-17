#### Configuration ################################################################################
_PROJ   = upnpd#                    		   # The name of the project and generated executable
_SDIR   = src#                                 # Path to directory of source files (relative to ./)
_HDIR   = include#                             # Path to directory of header files (relative to ./)
_BDIR   = bin#                                 # Path to directory of binary files (relative to ./)
_ODIR	= obj#								   # Path to obj directory
_ASMSUF = s#
_SSUF   = c#                                   # Suffix of source files
_SPPSUF = cpp#                                 # Suffix of source files
_HSUF   = h#                                   # Suffix of header files
_HPPSUF = hpp#                                 # Suffix of header files

PURPLE  = \033[0;35m#                          # Encoding of purple color for terminal output
CYAN    = \033[0;36m#                          # Encoding of cyan color for terminal output
NC      = \033[0m#                             # Encoding of no color for terminal output

# _CXX    = clang#                                 # Compiler to be used
# _CC		= clang

##### Build Configuration #########################################################################
_CPPFLAGS =
# _CXXFLAGS  = -G0 -Wall -Wextra -Wpedantic -std=c++17 -fno-exceptions -fno-rtti
_CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17

_LIBS = 

_INCLUDE = src/

_LDFLAGS = 
##### File Lists ##################################################################################
SRC_CPP = $(wildcard $(_SDIR)/*.$(_SPPSUF))
# SRC_C = $(wildcard $(_SDIR)/*.$(_SSUF))

HEDRS_CPP = $(wildcard $(_SDIR)/*.$(_HPPSUF))
# HEDRS_C = $(wildcard $(_SDIR)/*.$(_HSUF))

OBJS_CPP=$(addprefix $(_ODIR)/,$(notdir $(SRC_CPP:.$(_SPPSUF)=.o)))

##### Finalize Variables ###########################################################################
LIBS = $(addprefix -l,$(_LIBS))

INCLUDE = $(addprefix -I,$(_INCLUDE))

OBJS = $(OBJS_CPP)

CPPFLAGS = $(_CPPFLAGS)

CXXFLAGS = $(_CXXFLAGS) $(LIBS) $(INCLUDE)

LDFLAGS = $(_LDFLAGS)

##### Dependency Rules ############################################################################
.PHONY: run clean

all: prebuild_step $(_BDIR)/$(_PROJ) postbuild_step
##### User Rules ##################################################################################
prebuild_step: ;
#	@echo "Building..."


postbuild_step:
#	@echo "Finished building..."
#	@echo "Signing built executable ${CYAN}$(_BDIR)/$(_PROJ)${NC}..."
#	sudo codesign --sign - $(_BDIR)/$(_PROJ)
#	codesign -s "Michael Signing Identity" -f $(_BDIR)/$(_PROJ) --deep   
	sudo codesign -s - $(_BDIR)/$(_PROJ) --force 
##### Dependency Rules ############################################################################

# Link all compiled object files
$(_BDIR)/$(_PROJ): $(OBJS_CPP) $(OBJS_ASM)
	@echo "Linking object files ${PURPLE}$^${NC}..." 
	$(CXX) $(CARGS) -o $@ $^ 
	@echo "Successfully built executable ${CYAN}$@${NC}"

# Compile all outdated source files into their respective object files
$(_ODIR)/%.o: $(_SDIR)/%.$(_SPPSUF) $(HEDRS_CPP) | $(_ODIR) $(_BDIR)
	@echo "Compiling C++ source file ${PURPLE}$<${NC}..."
	$(CXX) $(CXXFLAGS) -c -o $@ $< 


# Ensure target folders for binaries exist and run any additional user defined shell script
$(_ODIR):
	mkdir -p $(_ODIR) 

$(_BDIR):
	mkdir -p $(_BDIR) 

clean:
	@echo "Cleaning up..." 
	rm -rf $(_BDIR) $(_ODIR) $(_SDIR)/*~ $(_HDIR)/*~ $(TARGET)

run: all
	$(_BDIR)/$(_PROJ)
