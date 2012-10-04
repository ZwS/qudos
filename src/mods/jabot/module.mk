jabot_SRCS=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/jabot/*.c)
jabot_SRCS+=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/jabot/ai/*.c)
jabot_OBJS=	$(patsubst %,$(OBJ_DIR)/jabot/%,$(notdir $(jabot_SRCS:%.c=%.o)))

$(OBJ_DIR)/jabot/.depend: $(jabot_SRCS)
	@mkdir -p $(OBJ_DIR)/jabot $(BIN_DIR)/jabot
	$(CC) -MM $(CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/jabot/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(CFLAGS) $$(SHLIB_CFLAGS) -o $$@ $$<' > $@

$(BIN_DIR)/jabot/$(GAME_NAME): $(jabot_OBJS)
	$(CC) -o $@ $^ $(SHLIB_LDFLAGS) $(LDFLAGS)
