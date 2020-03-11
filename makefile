## CONFIGURATION

# Compiler and flags
CC	= gcc
FLAGS	= -Wall -lpthread

# Directories
OBJDIR     = build
SRCDIR     = src
BINDIR     = bin

# Object files
SHARED_OBJFILES = utils.o llist.o
CLIENT_OBJFILES = client_main.o message.o $(SHARED_OBJFILES)
SERVER_OBJFILES = server_main.o $(SHARED_OBJFILES)

# Binary files
CLIENT = lotto_client
SERVER = lotto_server 

### EXECUTION

# Builds the executables: default rule
.PHONY: all
all : $(BINDIR)/$(CLIENT) $(BINDIR)/$(SERVER)

# Deletes objectfile and executables
.PHONY: clean
clean:
	rm -r $(OBJDIR)/* $(BINDIR)/*

# Build generic .o file from .c file
$(OBJDIR)/%.o: $(SRCDIR)/*/%.c
	$(CC) $(FLAGS) -c $< -o $@


$(BINDIR)/$(CLIENT): $(addprefix $(OBJDIR)/,$(CLIENT_OBJFILES))
	${CC} -o $@ $^ $(FLAGS)

	
$(BINDIR)/$(SERVER): $(addprefix $(OBJDIR)/,$(SERVER_OBJFILES))
	${CC} -o $@ $^ $(FLAGS)


