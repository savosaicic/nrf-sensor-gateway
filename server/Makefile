NAME			:= coap-server
CC				:= gcc
CPPFLAGS	:= -Iinclude
CFLAGS		:= -Wall -Wextra -Werror
LDFLAGS  	:= -lsqlite3 -lcoap-3 -lcjson
SRCS      := main.c db.c sensor.c coap_server.c snapshot_parser.c
OBJDIR 		:= obj
DEPDIR 		:= dep
OBJS      := $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))
DEPS			:= $(addprefix $(DEPDIR)/, $(SRCS:.c=.d))

vpath %.c src

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: %.c | $(OBJDIR) $(DEPDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(DEPDIR)/$*.d -c $< -o $@

ifneq "$(MAKECMDGOALS)" "clean"
 -include $(DEPS)
endif

debug: CFLAGS += -g -DDEBUG=1
debug: $(NAME)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(DEPDIR):
	mkdir -p $(DEPDIR)

clean:
	rm -rf $(OBJDIR) $(DEPDIR)

fclean: clean
	$(RM) -rf $(NAME)

re: fclean all

.PHONY: all debug clean fclean re
