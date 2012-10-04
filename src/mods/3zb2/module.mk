3zb2_SRCS=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/3zb2/*.c)
3zb2_OBJS=	$(patsubst %,$(OBJ_DIR)/3zb2/%,$(notdir $(3zb2_SRCS:%.c=%.o)))

$(OBJ_DIR)/3zb2/.depend: $(3zb2_SRCS)
	@mkdir -p $(OBJ_DIR)/3zb2 $(BIN_DIR)/3zb2
	$(CC) -MM $(CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/3zb2/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(CFLAGS) $$(SHLIB_CFLAGS) -o $$@ $$<' > $@

$(BIN_DIR)/3zb2/$(GAME_NAME): $(3zb2_OBJS)
	$(CC) -o $@ $^ $(SHLIB_LDFLAGS) $(LDFLAGS)
