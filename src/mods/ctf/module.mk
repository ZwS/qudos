ctf_SRCS=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/ctf/*.c)
ctf_OBJS=	$(patsubst %,$(OBJ_DIR)/ctf/%,$(notdir $(ctf_SRCS:%.c=%.o)))

$(OBJ_DIR)/ctf/.depend: $(ctf_SRCS)
	@mkdir -p $(OBJ_DIR)/ctf $(BIN_DIR)/ctf
	$(CC) -MM $(CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/ctf/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(CFLAGS) $$(SHLIB_CFLAGS) -o $$@ $$<' > $@

$(BIN_DIR)/ctf/$(GAME_NAME): $(ctf_OBJS)
	$(CC) -o $@ $^ $(SHLIB_LDFLAGS) $(LDFLAGS)
